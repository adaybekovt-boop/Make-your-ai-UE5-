#include "Campaign/MaiCampaign.h"
#include <algorithm>
#include <set>

namespace mai {
bool Campaign::Validate(std::string& error) const {
    const auto bad=[&](const char* message){error=message;return false;};
    if(!rules_.Valid(error) || !core_.Validate(core_.View(),error)) return false;
    const auto& s=state_;const auto now=core_.View().now;
    auto enumIn=[](auto v,int max){return static_cast<int>(v)>=0 && static_cast<int>(v)<=max;};
    auto quality=[](const DatasetQuality& q){return q.scoreBps>=0 && q.scoreBps<=10000 && q.noiseBps>=0 && q.noiseBps<=10000 && q.legalBps>=0 && q.legalBps<=10000 && q.acceptedBps>=0 && q.acceptedBps<=10000;};
    if(!enumIn(s.screen,10) || !enumIn(s.firstScreen,10) || !enumIn(s.lastScreen,10) || !enumIn(s.resumeScreen,10) || s.lastScreen!=s.screen || !enumIn(s.loading.phase,3) || !enumIn(s.loading.destination,10)) return bad("Invalid screen or loading state");
    if(s.loading.progressBps < -1 || s.loading.progressBps>10000 || s.loading.operation.size()>512 || s.loading.error.size()>512 || s.loading.interior.size()>128 || s.loading.generation>1000000000) return bad("Invalid loading record");
    if((s.loading.phase==LoadPhase::Loading || s.loading.phase==LoadPhase::Failed) && s.screen!=Screen::Loading) return bad("Pending loading must own the screen");
    if(s.screen==Screen::Loading && s.loading.phase!=LoadPhase::Loading && s.loading.phase!=LoadPhase::Failed) return bad("Loading screen has no pending operation");
    if(s.prologueStep<0 || s.prologueStep>3 || s.companyName.size()>80 || s.difficulty.size()>32 || s.interior.size()>128 || !s.seed || !s.reviewRng || !s.legalRng) return bad("Invalid campaign identity");
    if(!s.difficulty.empty() && !Difficulty()) return bad("Unknown saved difficulty");
    if(s.difficulty.empty() && (s.prologueStep!=0 || s.inventory.sequence!=0 || s.modelMicroIQ!=0)) return bad("Gameplay data before difficulty selection");
    if((s.screen==Screen::CityMap || s.screen==Screen::Gameplay || s.screen==Screen::Training || s.screen==Screen::Ending || s.screen==Screen::Results) && (!Difficulty() || s.prologueStep!=3)) return bad("Gameplay before prologue completion");
    if(s.walk.xCm<-800 || s.walk.xCm>800 || s.walk.yCm<-800 || s.walk.yCm>800 || s.walk.zCm<0 || s.walk.zCm>250 || s.walk.facingDeg<-180 || s.walk.facingDeg>180) return bad("Invalid walk pose");
    if(s.screen==Screen::Gameplay && s.interior.empty()) return bad("Gameplay walk screen requires an owned interior");
    if(!s.interior.empty() && core_.LocationStatus(s.interior)!=Status::Owned) return bad("Saved interior is not owned");
    if(s.screen==Screen::Ending && (!core_.View().ended || s.ending.kind==EndingKind::None || s.ending.offerOnly)) return bad("Ending screen without a committed ending");
    if(core_.View().ended && s.ending.kind==EndingKind::None) return bad("Ended company without ending information");
    if(s.debt<0 || s.debt>Dollars(20000) || s.modelMicroIQ<0 || s.modelMicroIQ>1000000000000LL || s.fixedCarry<0 || s.fixedCarry>=Hour/60 || s.insolventSince < -1 || s.insolventSince>now || s.lastLegalDay<0 || s.lastLegalDay>(now+8*Hour)/(24*Hour)) return bad("Invalid campaign finance or time");
    for(int v:{s.legalExposureBps,s.dependencyBps,s.employeeCareBps}) if(v<0 || v>10000) return bad("Invalid company exposure");
    if(s.warnings<0 || s.warnings>1000000 || s.ignoredWarnings<0 || s.ignoredWarnings>s.warnings || s.reward < -2048 || s.reward>2048) return bad("Invalid warnings or review reward");
    if(s.inventory.batches.size()>512 || s.reviews.size()>512 || s.training.size()>512 || s.specialists.size()>4 || s.decisions.size()>1024 || s.ending.decisions.size()>1024) return bad("Campaign collection limit");
    for(auto seq:{s.inventory.sequence,s.reviewSequence,s.trainingSequence,s.specialistSequence}) if(seq<0 || seq>1000000000) return bad("Invalid campaign sequence");
    std::set<std::int64_t> batches,reviews,jobs,staff;
    for(const auto& b:s.inventory.batches) {
        const DatasetOffer* offer=nullptr;for(const auto& o:rules_.offers) if(o.id==b.offerId) offer=&o;
        if(!offer || b.id<=0 || b.id>s.inventory.sequence || !batches.insert(b.id).second || b.type!=offer->type || b.volume!=offer->volume || b.cost<=0 || b.cost>MoneyLimit || !quality(b.original) || !quality(b.quality) || !enumIn(b.status,6) || !enumIn(b.method,3) || b.purchasedAt<0 || b.purchasedAt>now || b.reviewTime<0 || b.reviewTime>now || b.reviewId<0 || b.trainingId<0 || b.quality.legalBps!=b.original.legalBps || b.original.legalBps!=offer->quality.legalBps) return bad("Invalid dataset batch/provenance");
        if(b.status==DatasetStatus::Unreviewed && (b.method!=ReviewMethod::None || b.reviewId || b.trainingId)) return bad("Unreviewed batch already has work history");
        if(b.status>=DatasetStatus::Reviewing && (b.method==ReviewMethod::None || !Review(b.reviewId))) return bad("Missing review history");
    }
    for(const auto& h:s.specialists) if(h.id<=0 || h.id>s.specialistSequence || !staff.insert(h.id).second || h.accuracyBps<1 || h.accuracyBps>9900 || h.volumePerHour<1 || h.volumePerHour>1000 || h.fatigueBps<0 || h.fatigueBps>8000 || h.hireCost<0 || h.hireCost>MoneyLimit || h.batchFee<0 || h.batchFee>MoneyLimit) return bad("Invalid specialist");
    int activeManual=0,activeAI=0;std::set<std::int64_t> activeStaff;
    for(const auto& r:s.reviews) {
        const auto* b=Batch(r.batchId);
        if(!b || r.id<=0 || r.id>s.reviewSequence || !reviews.insert(r.id).second || b->reviewId!=r.id || r.method!=b->method || !enumIn(r.phase,2) || r.startedAt<0 || r.startedAt>now || r.elapsed<0 || r.elapsed>now || r.remainingMicro<0 || r.remainingMicro>static_cast<std::int64_t>(b->volume)*1000000 || r.carry<0 || r.carry>=Hour || r.resultQualityBps<0 || r.resultQualityBps>10000 || r.reward < -2 || r.reward>2) return bad("Invalid review session");
        if(r.phase==ReviewPhase::Complete && (r.remainingMicro!=0 || b->status==DatasetStatus::Reviewing || b->status==DatasetStatus::Unreviewed)) return bad("Review completion/status mismatch");
        if(r.phase!=ReviewPhase::Complete && b->status!=DatasetStatus::Reviewing) return bad("Active review has no Reviewing batch");
        if(r.method==ReviewMethod::Manual) {
            if(r.phase==ReviewPhase::Queued || r.items.size()!=static_cast<std::size_t>(rules_.manualSteps) || r.decisions.size()>r.items.size()) return bad("Invalid manual review length");
            if(r.phase==ReviewPhase::Active) ++activeManual;
            if(r.phase==ReviewPhase::Complete && r.decisions.size()!=r.items.size()) return bad("Incomplete manual decisions");
            std::set<std::string> itemIds;
            for(std::size_t i=0;i<r.items.size();++i) {
                const auto& item=r.items[i];bool known=false;for(const auto& def:rules_.items) if(def.id==item.id && def.type==item.type) known=true;
                if(!known || !itemIds.insert(item.id).second || item.betterSide<0 || item.betterSide>2 || item.left.size()>2048 || item.right.size()>2048 || item.prompt.size()>512 || (b->type!=DataType::Mixed && item.type!=b->type)) return bad("Invalid manual card");
                if(item.betterSide<0 || item.betterSide>2) return bad("Invalid manual card better-side");
                if(i<r.decisions.size()) {const auto& d=r.decisions[i];if(d.itemId!=item.id || d.side<0 || d.side>2 || d.correct!=(d.side==item.betterSide) || d.at<r.startedAt || d.at>now || (i>0 && d.at<r.decisions[i-1].at)) return bad("Invalid manual review decision");}
            }
        } else if(!r.items.empty() || !r.decisions.empty()) return bad("Automated review contains manual decisions");
        if(r.method==ReviewMethod::AI && (!s.ai.created || (r.phase==ReviewPhase::Active && ++activeAI>1))) return bad("Invalid AI review queue");
        if(r.method==ReviewMethod::Human && r.phase==ReviewPhase::Active && (!staff.count(r.specialistId) || !activeStaff.insert(r.specialistId).second)) return bad("Specialist capacity exceeded");
    }
    if(activeManual>1 || activeAI>1) return bad("Review concurrency exceeded");
    if(s.ai.level<1 || s.ai.level>5 || s.ai.accuracyBps<1 || s.ai.accuracyBps>9900 || s.ai.biasBps<0 || s.ai.biasBps>10000 || s.ai.computeMilli<1 || s.ai.computeMilli>100000 || s.ai.reviewed<0 || s.ai.reviewed>512) return bad("Invalid AI reviewer");
    int activeJobs=0;
    for(const auto& t:s.training) {
        const auto* b=Batch(t.batchId);
        if(!b || t.id<=0 || t.id>s.trainingSequence || !jobs.insert(t.id).second || b->trainingId!=t.id || !enumIn(t.phase,2) || t.remainingMicro<0 || t.remainingMicro>static_cast<std::int64_t>(b->volume)*1000000 || t.carry<0 || t.carry>=Hour || t.expectedGainMicroIQ<0 || t.expectedGainMicroIQ>1000000000 || t.startedAt<0 || t.startedAt>now || t.elapsed<0 || t.elapsed>now) return bad("Invalid training job");
        if(t.phase==JobPhase::Complete) {if(t.remainingMicro!=0 || b->status!=DatasetStatus::Trained) return bad("Invalid trained batch");}
        else {++activeJobs;if(b->status!=DatasetStatus::Training) return bad("Active job has no Training batch");}
    }
    if(activeJobs>1) return bad("Training concurrency exceeded");
    for(const auto& b:s.inventory.batches) if((b.status==DatasetStatus::Training || b.status==DatasetStatus::Trained) && !jobs.count(b.trainingId)) return bad("Missing training job");
    for(const auto& d:s.decisions) if(d.action.empty() || d.action.size()>128 || d.detail.size()>2048 || d.at<0 || d.at>now) return bad("Invalid decision journal");
    if(!enumIn(s.ending.kind,5) || s.ending.id.size()>128 || s.ending.title.size()>256 || s.ending.line.size()>2048 || s.ending.deal<0 || s.ending.deal>MoneyLimit) return bad("Invalid ending snapshot");
    return true;
}
} // namespace mai
