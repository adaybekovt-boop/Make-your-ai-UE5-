#include "Campaign/MaiCampaign.h"
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace mai {
namespace {
// Bounded, versioned, text wire format. A checksum detects accidental damage;
// this is NOT a cryptographic signature or a defense against a local save editor.
std::uint32_t Hash(const std::string& text) {std::uint32_t h=2166136261u;for(unsigned char b:text){h^=b;h*=16777619u;}return h;}
struct Writer;
struct Reader;
template<class A> void Fields(A& a,DatasetQuality& v) {a(v.scoreBps,v.noiseBps,v.legalBps,v.acceptedBps);}
template<class A> void Fields(A& a,DatasetItem& v) {a(v.id,v.prompt,v.left,v.right,v.type,v.betterSide,v.leftAsset,v.rightAsset);}
template<class A> void Fields(A& a,ReviewDecision& v) {a(v.itemId,v.side,v.correct,v.at);}
template<class A> void Fields(A& a,DatasetBatch& v) {a(v.id,v.offerId,v.type,v.volume,v.cost,v.original,v.quality,v.status,v.method,v.purchasedAt,v.reviewTime,v.reviewId,v.trainingId);}
template<class A> void Fields(A& a,DatasetInventory& v) {a(v.sequence,v.batches);}
template<class A> void Fields(A& a,ReviewSession& v) {a(v.id,v.batchId,v.specialistId,v.method,v.phase,v.items,v.decisions,v.startedAt,v.elapsed,v.remainingMicro,v.carry,v.resultQualityBps,v.reward);}
template<class A> void Fields(A& a,Specialist& v) {a(v.id,v.accuracyBps,v.volumePerHour,v.fatigueBps,v.hireCost,v.batchFee);}
template<class A> void Fields(A& a,AIReviewer& v) {a(v.created,v.accuracyBps,v.biasBps,v.level,v.computeMilli,v.reviewed);}
template<class A> void Fields(A& a,TrainingJob& v) {a(v.id,v.batchId,v.remainingMicro,v.carry,v.expectedGainMicroIQ,v.startedAt,v.elapsed,v.phase);}
template<class A> void Fields(A& a,DecisionRecord& v) {a(v.action,v.detail,v.at);}
template<class A> void Fields(A& a,LoadingState& v) {a(v.phase,v.generation,v.destination,v.interior,v.operation,v.error,v.progressBps);}
template<class A> void Fields(A& a,EndingMetrics& v) {a(v.cash,v.value,v.debt,v.profitPerHour,v.modelMicroIQ,v.reputation,v.legalBps,v.dataQualityBps,v.dependencyBps,v.employeeCareBps,v.automationBps,v.successfulReviews,v.ignoredWarnings,v.international,v.saleAccepted,v.saleDeclined,v.openChosen,v.bankrupt,v.difficulty);}
template<class A> void Fields(A& a,EndingResult& v) {a(v.kind,v.id,v.title,v.line,v.deal,v.offerOnly,v.allowReturnToSave,v.metrics,v.decisions);}
template<class A> void Fields(A& a,WalkState& v) {a(v.xCm,v.yCm,v.zCm,v.facingDeg);}
template<class A> void Fields(A& a,CampaignState& v) {
    a(v.screen);
    if(a.version>=2) a(v.firstScreen,v.lastScreen); // Explicit v1 -> v2 migration point.
    else {v.firstScreen=Screen::Loading;v.lastScreen=v.screen;}
    if(a.version>=3) a(v.resumeScreen);
    else v.resumeScreen=v.lastScreen==Screen::Loading?Screen::MainMenu:v.lastScreen;
    a(v.loading,v.difficulty,v.companyName,v.interior,v.prologueStep,v.seed,v.reviewRng,v.legalRng,
      v.inventory,v.reviews,v.specialists,v.ai,v.training,v.decisions,v.reviewSequence,v.specialistSequence,v.trainingSequence,v.modelMicroIQ,
      v.debt,v.legalExposureBps,v.dependencyBps,v.employeeCareBps,v.ignoredWarnings,v.warnings,v.reward,
      v.overwork,v.saleAccepted,v.saleDeclined,v.openChosen,v.insolventSince,v.fixedCarry,v.lastLegalDay,v.ending);
    if(a.version>=3) a(v.walk,v.quitRequested);
    else {v.walk={0,0,90,0};v.quitRequested=false;}
    if(v.difficulty=="Startup") v.difficulty="Easy";
    else if(v.difficulty=="Standard") v.difficulty="Normal";
    else if(v.difficulty=="Hardcore") v.difficulty="Hard";
}
struct Writer {
    std::ostream& stream;int version=2;
    template<class... T> void operator()(T&... values) {(One(values),...);}
    template<class T> void One(T& value) {
        if constexpr(std::is_enum_v<T>) stream<<static_cast<int>(value)<<' ';
        else if constexpr(std::is_arithmetic_v<T>) stream<<value<<' ';
        else Fields(*this,value);
    }
    void One(std::string& value) {stream<<std::quoted(value)<<' ';}
    template<class T> void One(std::vector<T>& values) {stream<<values.size()<<' ';for(auto& value:values) One(value);}
};
struct Reader {
    std::istream& stream;int version=2;bool ok=true;
    template<class... T> void operator()(T&... values) {(One(values),...);}
    template<class T> void One(T& value) {
        if(!ok) return;
        if constexpr(std::is_enum_v<T>) {int raw=0;ok=static_cast<bool>(stream>>raw);value=static_cast<T>(raw);}
        else if constexpr(std::is_arithmetic_v<T>) ok=static_cast<bool>(stream>>value);
        else Fields(*this,value);
    }
    void One(std::string& value) {if(ok) ok=static_cast<bool>(stream>>std::quoted(value)) && value.size()<=4096;}
    template<class T> void One(std::vector<T>& values) {
        if(!ok) return;
        std::size_t count=0;ok=static_cast<bool>(stream>>count) && count<=1024;if(!ok) return;
        values.resize(count);for(auto& value:values) One(value);
    }
};
}
std::string Campaign::Save() const {
    std::ostringstream body;body<<std::quoted(core_.Save())<<' ';
    auto copy=state_;Writer writer{body,3};writer(copy);
    const auto payload=body.str();std::ostringstream out;
    out<<"MAI-CAMPAIGN 3 "<<base_.Fingerprint()<<' '<<rules_.Fingerprint()<<' '<<Hash(payload)<<'\n'<<payload;
    return out.str();
}
Result Campaign::ReadBody(const std::string& body,int version) {
    std::istringstream input(body);std::string core;
    if(!(input>>std::quoted(core)) || core.size()>1024*1024) return Result::Error("Invalid nested company snapshot");
    Reader reader{input,version,true};reader(state_);input>>std::ws;
    if(!reader.ok || !input.eof()) return Result::Error("Truncated or trailing campaign data");
    if(!state_.difficulty.empty() && !Difficulty()) return Result::Error("Unknown difficulty in save");
    core_=Simulation(Difficulty()?ApplyDifficulty(base_,*Difficulty()):base_,state_.seed);
    auto r=core_.Load(core);if(!r.ok) return r;
    std::string error;if(!Validate(error)) return Result::Error(error);return Result::Success();
}
Result Campaign::Load(const std::string& encoded) {
    if(encoded.size()>8*1024*1024) return Result::Error("Campaign save exceeds 8 MiB limit");
    Campaign candidate(base_,rules_,42);
    if(encoded.rfind("MAI-SAVE ",0)==0) {
        // Migrate the earlier native procurement-only schema. Browser IndexedDB
        // saves remain a different format; no unsupported conversion is claimed.
        auto r=candidate.core_.Load(encoded);if(!r.ok) return r;
        if(candidate.core_.View().ended) return Result::Error("Legacy ended save lacks an ending snapshot; explicit migration is required");
        auto& s=candidate.state_;s.difficulty="Normal";s.companyName="Migrated company";s.prologueStep=3;
        s.screen=Screen::CityMap;s.lastScreen=s.screen;s.resumeScreen=s.screen;s.loading.phase=LoadPhase::Ready;s.loading.destination=s.screen;
        s.loading.progressBps=10000;s.lastLegalDay=(candidate.core_.View().now+8*Hour)/(24*Hour);s.walk={0,0,90,0};
        candidate.Record("migration","Native procurement schema 1 -> campaign schema 3; Normal difficulty retained");
    } else {
        const auto newline=encoded.find('\n');if(newline==std::string::npos) return Result::Error("Missing campaign save header");
        std::istringstream header(encoded.substr(0,newline));std::string magic;int version=0;std::uint32_t base=0,rules=0,checksum=0;
        if(!(header>>magic>>version>>base>>rules>>checksum) || magic!="MAI-CAMPAIGN" || (version<1 || version>3)) return Result::Error("Unsupported campaign schema");
        header>>std::ws;if(!header.eof()) return Result::Error("Unexpected campaign header data");
        if(base!=base_.Fingerprint() || rules!=rules_.Fingerprint()) return Result::Error("Rules changed; a deliberate migration is required");
        const auto body=encoded.substr(newline+1);if(Hash(body)!=checksum) return Result::Error("Campaign checksum mismatch");
        auto r=candidate.ReadBody(body,version);if(!r.ok) return r;
    }
    std::string error;if(!candidate.Validate(error)) return Result::Error(error);
    const auto keepSettings=settings_;
    *this=std::move(candidate);
    settings_=keepSettings;
    return Result::Success("Campaign restored atomically; no offline time advanced");
}
} // namespace mai
