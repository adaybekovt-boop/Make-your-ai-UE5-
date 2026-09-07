#include "Campaign/MaiCampaign.h"
#include <algorithm>
#include <set>

namespace mai {
namespace {
std::int64_t Bps(std::int64_t value,int multiplier) {return value/10000*multiplier+(value%10000)*multiplier/10000;}
int ClampBps(int value) {return std::clamp(value,0,10000);}
bool IsPlayable(Screen s) {return s==Screen::CityMap || s==Screen::Gameplay || s==Screen::Training;}
bool IsMenuScreen(Screen s) {return s==Screen::MainMenu || s==Screen::Results || s==Screen::Settings || s==Screen::Ending;}
}
Catalog Campaign::ApplyDifficulty(Catalog base,const DifficultyProfile& p) {
    base.defectPpm=static_cast<int>(std::min<std::int64_t>(1000000,Bps(base.defectPpm,p.defectBps)));
    base.failurePpm=static_cast<int>(std::min<std::int64_t>(1000000,Bps(base.failurePpm,p.failureBps)));
    for(auto& c:base.chips) c.price=Dollars((c.price/Unit*p.procurementBps+5000)/10000);
    for(auto& c:base.chassis) c.price=Dollars((c.price/Unit*p.procurementBps+5000)/10000);
    for(auto& region:base.regions) region.courtBps=static_cast<int>(std::min<std::int64_t>(30000,Bps(region.courtBps,p.legalBps)));
    base.extensions.rivalBudget=std::min<Money>(Dollars(1000000),Bps(base.extensions.rivalBudget,p.competitorBps));
    base.extensions.rivalAggressionPpm=static_cast<int>(std::min<std::int64_t>(1000000,Bps(base.extensions.rivalAggressionPpm,p.competitorBps)));
    return base;
}
Campaign::Campaign(Catalog base,CampaignRules rules,std::uint32_t seed):base_(std::move(base)),rules_(std::move(rules)),core_(base_,seed) {Reset(seed);}
const DifficultyProfile* Campaign::Difficulty() const {for(const auto& d:rules_.difficulties) if(d.id==state_.difficulty) return &d;return nullptr;}
bool Campaign::CanPlay() const {return Difficulty() && state_.prologueStep==3 && IsPlayable(state_.screen) && state_.loading.phase!=LoadPhase::Loading && !core_.View().ended;}
void Campaign::Visit(Screen s) {state_.screen=s;state_.lastScreen=s;}
void Campaign::Record(const std::string& action,const std::string& detail) {
    state_.decisions.push_back({action,detail,core_.View().now});
    if(state_.decisions.size()>1024) state_.decisions.erase(state_.decisions.begin());
}
Result Campaign::Reset(std::uint32_t seed) {
    std::string error;if(!base_.Valid(error) || !rules_.Valid(error)) return Result::Error(error);
    const auto keepSettings=settings_;
    core_=Simulation(base_,seed);core_.SetPaused(true);state_=CampaignState{};
    settings_=keepSettings;state_.quitRequested=false;
    state_.seed=seed?seed:1;state_.reviewRng=state_.seed^0xa511e9b3u;if(!state_.reviewRng) state_.reviewRng=1;
    state_.legalRng=state_.seed^0x6c8e9cf5u;if(!state_.legalRng) state_.legalRng=1;
    return BeginLoad(Screen::MainMenu);
}
Result Campaign::BeginNewGame() {
    if(state_.screen!=Screen::MainMenu && state_.screen!=Screen::Ending && state_.screen!=Screen::Results) return Result::Error("Start a new company from the menu, ending or results");
    const auto generation=state_.loading.generation;
    const auto seed=state_.seed;
    auto r=Reset(seed);if(!r.ok) return r;
    state_.loading.generation=generation+1;state_.loading.phase=LoadPhase::Idle;Visit(Screen::NewGame);
    return Result::Success("New company; existing save slots are untouched");
}
Result Campaign::ShowDifficulty() {if(state_.screen!=Screen::NewGame) return Result::Error("New Game must precede Difficulty");Visit(Screen::Difficulty);return Result::Success();}
Result Campaign::ChooseDifficulty(const std::string& id) {
    if(state_.screen!=Screen::Difficulty || !state_.difficulty.empty()) return Result::Error("Difficulty is locked for the lifetime of this company");
    const DifficultyProfile* found=nullptr;for(const auto& p:rules_.difficulties) if(p.id==id) found=&p;
    if(!found) return Result::Error("Unknown difficulty");
    auto catalog=ApplyDifficulty(base_,*found);std::string error;if(!catalog.Valid(error)) return Result::Error(error);
    Simulation fresh(catalog,state_.seed);auto s=fresh.View();s.cash=Bps(Dollars(12000),found->capitalBps);s.paused=true;
    auto r=fresh.Restore(s);if(!r.ok) return r;
    core_=std::move(fresh);state_.difficulty=id;Visit(Screen::Prologue);Record("difficulty",id);return Result::Success("Difficulty selected and locked");
}
Result Campaign::PrologueAction(const std::string& input) {
    if(state_.screen!=Screen::Prologue) return Result::Error("Prologue is not active");
    if(state_.prologueStep==0) {
        if(input.empty() || input.size()>80 || input.find_first_of("\r\n\t")!=std::string::npos) return Result::Error("Company name must be 1-80 bytes on one line");
        state_.companyName=input;state_.prologueStep=1;Record("create-company",input);return Result::Success("Company created; inspect the starter budget");
    }
    if(state_.prologueStep==1) {
        if(input!="budget") return Result::Error("Inspect the server, data and specialist costs first");
        state_.prologueStep=2;Record("budget","Equipment, licensed data and specialists all cost money; no starter equipment was given for free");return Result::Success("First task: acquire Garage, install a server, review a dataset sample");
    }
    if(state_.prologueStep==2 && input=="accept-task") {
        state_.prologueStep=3;core_.SetPaused(false);Record("first-task","Garage / first reviewed dataset / first training job");return BeginLoad(Screen::CityMap);
    }
    return Result::Error("Accept the first task");
}
Result Campaign::BeginLoad(Screen destination,const std::string& interior) {
    if(state_.loading.phase==LoadPhase::Loading) return Result::Error("A mandatory load is already running");
    if(destination!=Screen::MainMenu && destination!=Screen::CityMap && destination!=Screen::Gameplay && destination!=Screen::Training) return Result::Error("Unsupported load destination");
    if(destination!=Screen::MainMenu && (!Difficulty() || state_.prologueStep!=3 || core_.View().ended)) return Result::Error("Finish new-company setup before loading gameplay");
    if(!interior.empty()) {
        if(core_.LocationStatus(interior)!=Status::Owned) return Result::Error("Buy this location before entering");
        const auto profiles=InteriorProfiles();const auto it=std::find_if(profiles.begin(),profiles.end(),[&](const InteriorProfile& p){return p.id==interior;});
        if(it==profiles.end() || !it->walkable) return Result::Error("Interior data exists, but walking is not connected for this location yet");
    }
    state_.resumeScreen=IsPlayable(state_.screen)?state_.screen:(state_.screen==Screen::Loading?state_.resumeScreen:Screen::MainMenu);
    const auto next=state_.loading.generation+1;
    state_.loading={LoadPhase::Loading,next,destination,interior,"Preparing required assets",{},-1};Visit(Screen::Loading);
    return Result::Success("Mandatory loading started");
}
Result Campaign::ReportLoading(std::uint64_t generation,int progress,const std::string& operation) {
    if(generation!=state_.loading.generation || state_.loading.phase!=LoadPhase::Loading) return Result::Error("Stale load callback ignored");
    if(progress < -1 || progress>10000 || operation.size()>512) return Result::Error("Invalid load progress");
    state_.loading.progressBps=progress;state_.loading.operation=operation;return Result::Success();
}
Result Campaign::CompleteLoad(std::uint64_t generation,bool success,const std::string& error) {
    if(generation!=state_.loading.generation || state_.loading.phase!=LoadPhase::Loading) return Result::Error("Stale load completion ignored");
    if(!success) {state_.loading.phase=LoadPhase::Failed;state_.loading.error=error.empty()?"Required asset or scene failed to load":error.substr(0,512);return Result::Error(state_.loading.error);}
    state_.loading.phase=LoadPhase::Ready;state_.loading.progressBps=10000;state_.loading.error.clear();
    if(state_.loading.destination==Screen::CityMap) state_.interior.clear();
    else state_.interior=state_.loading.interior;
    if(state_.loading.destination==Screen::Gameplay && state_.walk.xCm==0 && state_.walk.yCm==0 && state_.walk.zCm==0) state_.walk={0,0,90,0};
    Visit(state_.loading.destination);
    return Result::Success("Required loading and scene construction completed");
}
Result Campaign::CancelLoad() {
    if(state_.loading.phase!=LoadPhase::Loading && state_.loading.phase!=LoadPhase::Failed) return Result::Error("No load to cancel");
    if(state_.loading.destination==Screen::MainMenu && state_.loading.phase==LoadPhase::Loading) return Result::Error("Boot load cannot be cancelled");
    if(state_.loading.phase==LoadPhase::Failed) {
        state_.loading={LoadPhase::Ready,state_.loading.generation,Screen::MainMenu,{},{},{},10000};
        Visit(Screen::MainMenu);return Result::Success("Failed load dismissed; returned to main menu");
    }
    if(!IsPlayable(state_.resumeScreen)) return Result::Error("No safe screen to restore; cancel refused");
    state_.loading.phase=LoadPhase::Ready;state_.loading.error.clear();
    Visit(state_.resumeScreen);return Result::Success("Optional load cancelled; previous playable screen restored");
}
Result Campaign::RetryLoad() {
    if(state_.loading.phase!=LoadPhase::Failed) return Result::Error("Retry requires a failed load");
    const auto dest=state_.loading.destination;const auto interior=state_.loading.interior;
    state_.loading.phase=LoadPhase::Idle;return BeginLoad(dest,interior);
}
Result Campaign::ShowScreen(Screen screen) {
    if(screen==Screen::Settings && (state_.screen==Screen::MainMenu || state_.screen==Screen::Settings)) {Visit(screen);return Result::Success();}
    if(screen==Screen::MainMenu && (state_.screen==Screen::Settings || state_.screen==Screen::Results || state_.screen==Screen::Ending || state_.screen==Screen::NewGame)) {Visit(screen);return Result::Success();}
    if(screen==Screen::MainMenu && state_.screen!=Screen::Loading) {Visit(screen);return Result::Success();}
    if(!CanPlay() || (screen!=Screen::Training && screen!=Screen::Gameplay && screen!=Screen::CityMap)) return Result::Error("Screen transition requires active gameplay");
    if(screen==Screen::Gameplay && state_.interior.empty()) return Result::Error("Enter a walkable interior first");
    Visit(screen);return Result::Success();
}
Result Campaign::LoadFromMenu(const std::string& encoded) {
    if(state_.screen!=Screen::MainMenu && state_.screen!=Screen::Results) return Result::Error("Load Game is only available from the main menu or results");
    return Load(encoded);
}
Result Campaign::RequestQuit() {
    if(!IsMenuScreen(state_.screen) && state_.screen!=Screen::Loading) return Result::Error("Quit is available from menu screens");
    state_.quitRequested=true;Record("quit","Host should exit; company state is unchanged");return Result::Success("Quit requested");
}
Result Campaign::SetTextScale(int textScaleBps) {
    if(textScaleBps<5000 || textScaleBps>20000) return Result::Error("Text scale must be 50%-200%");
    settings_.textScaleBps=textScaleBps;return Result::Success();
}
Result Campaign::SetReducedMotion(bool enabled) {settings_.reducedMotion=enabled;return Result::Success();}
Result Campaign::Spend(Money amount,bool capital) {
    if(amount<0 || amount>MoneyLimit || core_.View().cash<amount) return Result::Error("Insufficient funds");
    auto s=core_.View();if((capital?s.capex:s.expenses)>MoneyLimit-amount) return Result::Error("Accounting limit");
    s.cash-=amount;if(capital) s.capex+=amount;else s.expenses+=amount;return core_.Restore(s);
}
const DatasetBatch* Campaign::Batch(std::int64_t id) const {for(const auto& b:state_.inventory.batches) if(b.id==id) return &b;return nullptr;}
DatasetBatch* Campaign::MutableBatch(std::int64_t id) {for(auto& b:state_.inventory.batches) if(b.id==id) return &b;return nullptr;}
const ReviewSession* Campaign::Review(std::int64_t id) const {for(const auto& r:state_.reviews) if(r.id==id) return &r;return nullptr;}
Result Campaign::BuyDataset(const std::string& offer) {
    if(!CanPlay()) return Result::Error("Dataset purchases require an active company");
    if(state_.inventory.batches.size()>=512) return Result::Error("Dataset inventory limit reached");
    const DatasetOffer* def=nullptr;for(const auto& d:rules_.offers) if(d.id==offer) def=&d;
    if(!def) return Result::Error("Unknown dataset offer");
    const Money price=Bps(def->cost,Difficulty()->procurementBps);auto payment=Spend(price);if(!payment.ok) return payment;
    DatasetBatch b;b.id=++state_.inventory.sequence;b.offerId=def->id;b.type=def->type;b.volume=def->volume;b.cost=price;b.original=def->quality;b.quality=b.original;b.purchasedAt=core_.View().now;
    Record("dataset-purchased",def->id);b.status=DatasetStatus::Unreviewed;state_.inventory.batches.push_back(b);Record("dataset-unreviewed",std::to_string(b.id));
    return Result::Success("Dataset entered Unreviewed inventory; it cannot train yet");
}
Result Campaign::HireSpecialist() {
    if(!CanPlay() || state_.specialists.size()>=4) return Result::Error("Hire at most four specialists during gameplay");
    const auto cost=Bps(rules_.humanHire,Difficulty()->hiringBps);auto r=Spend(cost);if(!r.ok) return r;
    state_.specialists.push_back({++state_.specialistSequence,rules_.humanAccuracyBps,rules_.humanVolumePerHour,0,cost,Bps(rules_.humanBatch,Difficulty()->hiringBps)});
    Record("hire-specialist","Per-batch payment; one active batch per specialist");return Result::Success("Specialist hired");
}
Result Campaign::CreateAIReviewer() {
    if(!CanPlay() || state_.ai.created) return Result::Error("AI reviewer already exists or company inactive");
    if(core_.Economy().computeMilli<500) return Result::Error("Install an operational server before creating the AI reviewer");
    auto r=Spend(rules_.aiSetup,true);if(!r.ok) return r;state_.ai.created=true;Record("create-ai-reviewer","Fast review with systematic bias; reserves 0.5 compute");return Result::Success("AI reviewer created");
}
Result Campaign::ImproveAIReviewer() {
    if(!CanPlay() || !state_.ai.created || state_.ai.level>=5) return Result::Error("AI improvement unavailable");
    auto r=Spend(rules_.aiUpgrade);if(!r.ok) return r;++state_.ai.level;state_.ai.accuracyBps=std::min(9800,state_.ai.accuracyBps+250);state_.ai.biasBps=std::max(200,state_.ai.biasBps-250);Record("improve-ai-reviewer",std::to_string(state_.ai.level));return Result::Success();
}
Result Campaign::SetOverwork(bool enabled) {if(!CanPlay()) return Result::Error("Company inactive");state_.overwork=enabled;Record("staff-policy",enabled?"overwork":"normal-hours");return Result::Success();}
Result Campaign::StartReview(std::int64_t batchId,ReviewMethod method) {
    if(!CanPlay()) return Result::Error("Company inactive");
    auto* batch=MutableBatch(batchId);if(!batch || batch->status!=DatasetStatus::Unreviewed) return Result::Error("Only an Unreviewed batch can enter review");
    if(state_.reviews.size()>=512 || method==ReviewMethod::None || static_cast<int>(method)>3 || static_cast<int>(method)<1) return Result::Error("Invalid review method or queue full");
    if(method==ReviewMethod::Manual && state_.interior!="garage") return Result::Error("Use the review desk inside Garage for manual review");
    if(method==ReviewMethod::Manual) for(const auto& r:state_.reviews) if(r.method==method && r.phase!=ReviewPhase::Complete) return Result::Error("Finish the active manual session first");
    if(method==ReviewMethod::Human && state_.specialists.empty()) return Result::Error("Hire a specialist first");
    if(method==ReviewMethod::AI && !state_.ai.created) return Result::Error("Create an AI reviewer first");
    if(method==ReviewMethod::Human) {auto r=Spend(state_.specialists.front().batchFee);if(!r.ok) return r;}
    ReviewSession review;review.id=++state_.reviewSequence;review.batchId=batchId;review.method=method;review.startedAt=core_.View().now;review.remainingMicro=static_cast<std::int64_t>(batch->volume)*1000000;
    if(method==ReviewMethod::Manual) {
        review.phase=ReviewPhase::Active;
        std::set<std::string> used;
        for(int step=0;step<rules_.manualSteps;++step) {
            const auto type=batch->type==DataType::Mixed?(step%2?DataType::Image:DataType::Text):batch->type;
            std::vector<DatasetItem> candidates;for(const auto& item:rules_.items) if(item.type==type && !used.count(item.id)) candidates.push_back(item);
            const auto index=Simulation::Roll(state_.reviewRng)%candidates.size();auto item=candidates[index];used.insert(item.id);
            if(item.betterSide<2 && Simulation::Roll(state_.reviewRng)<500000) {std::swap(item.left,item.right);std::swap(item.leftAsset,item.rightAsset);item.betterSide=1-item.betterSide;}
            review.items.push_back(std::move(item));
        }
    }
    batch->status=DatasetStatus::Reviewing;batch->method=method;batch->reviewId=review.id;state_.reviews.push_back(std::move(review));
    Record("start-review",std::to_string(batchId)+" method="+std::to_string(static_cast<int>(method)));return Result::Success("Review session created");
}
void Campaign::FinishReview(ReviewSession& r,int score) {
    auto* b=MutableBatch(r.batchId);if(!b || r.phase==ReviewPhase::Complete) return;
    r.phase=ReviewPhase::Complete;r.remainingMicro=0;r.resultQualityBps=ClampBps(score);
    b->reviewTime=r.elapsed;b->quality.scoreBps=r.resultQualityBps;
    b->quality.noiseBps=ClampBps(b->original.noiseBps-(score-b->original.scoreBps));
    b->quality.acceptedBps=std::clamp(5000+score/2,5000,9900);
    // Reviewing does not grant a license. Provenance survives every method.
    b->quality.legalBps=b->original.legalBps;
    b->status=score>=rules_.verifiedThresholdBps?DatasetStatus::Verified:DatasetStatus::Rejected;
    r.reward=b->status==DatasetStatus::Verified?2:-2;state_.reward+=r.reward;
    auto s=core_.View();s.reputation=std::clamp(s.reputation+r.reward,0,100);core_.Restore(s);
    Record("review-result",std::to_string(b->id)+" "+DatasetStatusName(b->status)+" quality="+std::to_string(score));
}
Result Campaign::ChooseReview(std::int64_t id,int side) {
    if(!CanPlay() || side<0 || side>2) return Result::Error("Invalid review choice");
    ReviewSession* r=nullptr;for(auto& v:state_.reviews) if(v.id==id) r=&v;
    if(!r || r->method!=ReviewMethod::Manual || r->phase!=ReviewPhase::Active || r->decisions.size()>=r->items.size()) return Result::Error("Manual review session is not active");
    const auto& item=r->items[r->decisions.size()];const bool correct=side==item.betterSide;
    r->decisions.push_back({item.id,side,correct,core_.View().now});Record("manual-choice",item.id+" side="+std::to_string(side));
    if(r->decisions.size()==r->items.size()) {
        int good=0;for(const auto& d:r->decisions) good+=d.correct?1:0;
        const auto* b=Batch(r->batchId);
        const int score=ClampBps(b->original.scoreBps+good*300-(static_cast<int>(r->items.size())-good)*1800);
        FinishReview(*r,std::min(9900,score));
    }
    return Result::Success(correct?"Choice recorded":"Choice recorded; this sample introduces noise");
}
Result Campaign::StartTraining(std::int64_t batchId) {
    if(!CanPlay()) return Result::Error("Company inactive");
    auto* b=MutableBatch(batchId);if(!b || b->status!=DatasetStatus::Verified) return Result::Error("Training requires a Verified batch");
    for(const auto& t:state_.training) if(t.phase!=JobPhase::Complete) return Result::Error("A training job is already active");
    if(AvailableTrainingCompute()<=0) return Result::Error("No server compute available after AI review reservation");
    TrainingJob t;t.id=++state_.trainingSequence;t.batchId=b->id;t.startedAt=core_.View().now;t.remainingMicro=static_cast<std::int64_t>(b->volume)*1000000;
    t.expectedGainMicroIQ=Bps(Bps(Bps(static_cast<std::int64_t>(b->volume)*rules_.microIQPerVolume,b->quality.scoreBps),b->quality.acceptedBps),10000-b->quality.noiseBps);
    b->status=DatasetStatus::Training;b->trainingId=t.id;state_.training.push_back(t);Visit(Screen::Training);Record("start-training",std::to_string(batchId));return Result::Success("Verified data assigned to the server training job");
}
Result Campaign::PauseTraining(bool paused) {
    if(!CanPlay()) return Result::Error("Company inactive");
    for(auto& t:state_.training) if(t.phase!=JobPhase::Complete) {t.phase=paused?JobPhase::Paused:JobPhase::Running;Record("training-pause",paused?"paused":"resumed");return Result::Success();}
    return Result::Error("No active training job");
}
std::int64_t Campaign::AvailableTrainingCompute() const {
    auto compute=core_.Economy().computeMilli;
    for(const auto& r:state_.reviews) if(r.method==ReviewMethod::AI && r.phase!=ReviewPhase::Complete) {compute-=state_.ai.computeMilli;break;}
    return std::max<std::int64_t>(0,compute);
}
Result Campaign::Remediate() {
    if(!CanPlay() || state_.legalExposureBps==0) return Result::Error("No active legal exposure to remediate");
    auto r=Spend(rules_.remediationCost);if(!r.ok) return r;
    state_.legalExposureBps=std::max(0,state_.legalExposureBps-2500);Record("legal-remediation","Audit and withdrawal of affected outputs; original provenance retained");return Result::Success();
}
Result Campaign::IgnoreWarning() {
    if(!CanPlay() || state_.ignoredWarnings>=state_.warnings) return Result::Error("No unread legal warning");
    ++state_.ignoredWarnings;Record("ignore-legal-warning",std::to_string(state_.ignoredWarnings));return EvaluateEnding();
}
Result Campaign::Borrow(Money amount) {
    if(!CanPlay() || amount<=0 || amount>Dollars(5000) || state_.debt>Dollars(20000)-amount || core_.View().cash>MoneyLimit-amount) return Result::Error("Loan limit or company restriction");
    auto s=core_.View();s.cash+=amount;auto r=core_.Restore(s);if(!r.ok) return r;state_.debt+=amount;state_.dependencyBps=static_cast<int>(state_.debt*10000/Dollars(20000));Record("competitor-backed-loan",FormatMoney(amount));return Result::Success("Debt recorded; loan principal is not revenue");
}
Result Campaign::Repay(Money amount) {
    if(!CanPlay() || amount<=0 || amount>state_.debt || core_.View().cash<amount) return Result::Error("Invalid repayment");
    auto s=core_.View();s.cash-=amount;auto r=core_.Restore(s);if(!r.ok) return r;state_.debt-=amount;state_.dependencyBps=static_cast<int>(state_.debt*10000/Dollars(20000));Record("repay",FormatMoney(amount));return Result::Success();
}
Result Campaign::TalkToGarageNpc() {
    if(!CanPlay() || state_.interior!="garage") return Result::Error("The advisor is inside Garage");
    Record("advisor","Order hardware once; mount only after delivery. Review data at the desk before training.");return Result::Success("Advisor: Buying data is not the same as checking it. The desk is over there.");
}
EndingMetrics Campaign::Metrics() const {
    EndingMetrics m;const auto& s=core_.View();const auto* d=Difficulty();const auto rates=core_.Economy();
    m.cash=s.cash;m.debt=state_.debt;m.reputation=s.reputation;m.modelMicroIQ=state_.modelMicroIQ;m.profitPerHour=rates.revenue-rates.Expenses();
    const Money earningsValue=std::min<Money>(Dollars(100000000),std::max<Money>(0,m.profitPerHour)*240);
    m.value=std::max<Money>(0,std::min<Money>(MoneyLimit,std::max<Money>(0,s.cash)+s.capex/2+earningsValue)-m.debt);
    m.legalBps=std::max(state_.legalExposureBps,core_.CourtRiskPpm()/100);m.dependencyBps=state_.dependencyBps;m.employeeCareBps=state_.employeeCareBps;
    int ai=0,complete=0;std::int64_t quality=0;
    for(const auto& r:state_.reviews) if(r.phase==ReviewPhase::Complete) {++complete;if(r.method==ReviewMethod::AI) ++ai;const auto* b=Batch(r.batchId);if(b && b->status!=DatasetStatus::Rejected){quality+=b->quality.scoreBps;++m.successfulReviews;}}
    m.automationBps=complete?ai*10000/complete:0;m.dataQualityBps=m.successfulReviews?static_cast<int>(quality/m.successfulReviews):0;
    for(std::size_t i=0;i<s.locations.size();++i) if(s.locations[i].owned && core_.Definitions().locations[i].region!="home") m.international=true;
    m.saleAccepted=state_.saleAccepted;m.saleDeclined=state_.saleDeclined;m.openChosen=state_.openChosen;m.ignoredWarnings=state_.ignoredWarnings;
    m.bankrupt=d && s.cash<=0 && state_.insolventSince>=0 && s.now-state_.insolventSince>=d->insolvencyGrace;
    m.difficulty=state_.difficulty;return m;
}
Result Campaign::EvaluateEnding() {
    if(!Difficulty() || state_.prologueStep!=3) return Result::Error("No active campaign to evaluate");
    if(core_.View().ended) return Result::Success("Ending is already final");
    auto result=EndingEvaluator::Evaluate(Metrics(),rules_.endings);
    if(result.kind==EndingKind::None) {state_.ending=result;return Result::Success("No ending qualifies; the company continues");}
    result.decisions=state_.decisions;state_.ending=std::move(result);
    if(state_.ending.offerOnly) return Result::Success("Elon Max has made an acquisition offer; accept or decline");
    auto s=core_.View();s.ended=true;s.paused=true;auto r=core_.Restore(s);if(!r.ok) return r;
    Visit(Screen::Ending);Record("ending",state_.ending.id);state_.ending.decisions=state_.decisions;return Result::Success(state_.ending.title);
}
Result Campaign::ChooseEnding(EndingKind choice) {
    if(!CanPlay() || (choice!=EndingKind::Acquisition && choice!=EndingKind::OpenModel)) return Result::Error("This ending is not a selectable decision");
    auto probe=Metrics();probe.saleAccepted=choice==EndingKind::Acquisition;probe.openChosen=choice==EndingKind::OpenModel;
    const auto evaluated=EndingEvaluator::Evaluate(probe,rules_.endings);
    if(evaluated.kind!=choice) return Result::Error("The company does not meet the selected ending requirements");
    if(choice==EndingKind::Acquisition) state_.saleAccepted=true;else state_.openChosen=true;
    Record(choice==EndingKind::Acquisition?"accept-acquisition":"open-model","Explicit player decision");return EvaluateEnding();
}
Result Campaign::DeclineSale() {if(!CanPlay()) return Result::Error("Company inactive");state_.saleDeclined=true;Record("decline-acquisition","Remain independent");return EvaluateEnding();}
Result Campaign::AcknowledgeEnding() {
    if(state_.screen!=Screen::Ending || state_.ending.kind==EndingKind::None || state_.ending.offerOnly) return Result::Error("No committed ending to acknowledge");
    Visit(Screen::Results);return Result::Success("Results recorded; start a new company to avoid leaking this session");
}
Result Campaign::SkipReview(std::int64_t id) {
    ReviewSession* r=nullptr;for(auto& v:state_.reviews) if(v.id==id) r=&v;
    if(!r || r->phase!=ReviewPhase::Active || r->method!=ReviewMethod::Manual || r->decisions.size()>=r->items.size()) return Result::Error("No active manual sample to skip");
    const int wrong=r->items[r->decisions.size()].betterSide==0?1:0;
    return ChooseReview(id,wrong);
}
Result Campaign::CancelReview(std::int64_t id) {
    if(!CanPlay()) return Result::Error("Company inactive");
    auto it=std::find_if(state_.reviews.begin(),state_.reviews.end(),[&](const ReviewSession& r){return r.id==id;});
    if(it==state_.reviews.end() || it->phase==ReviewPhase::Complete) return Result::Error("Review session is not cancellable");
    if(it->method!=ReviewMethod::Manual) return Result::Error("Human and AI reviews cannot be cancelled after payment; wait for completion");
    auto* batch=MutableBatch(it->batchId);if(!batch) return Result::Error("Missing batch for cancelled review");
    batch->status=DatasetStatus::Unreviewed;batch->method=ReviewMethod::None;batch->reviewId=0;batch->reviewTime=0;
    state_.reviews.erase(it);Record("cancel-manual-review",std::to_string(id));
    return Result::Success("Manual review cancelled; the batch returned to Unreviewed and no fee was charged");
}
Result Campaign::EnterWalk(const std::string& interior) {return BeginLoad(Screen::Gameplay,interior.empty()?"garage":interior);}
Result Campaign::ReturnToCity() {
    if(!CanPlay() || state_.interior.empty()) return Result::Error("Already on the city map");
    return BeginLoad(Screen::CityMap);
}
Result Campaign::SetWalkPosition(int xCm,int yCm,int zCm,int facingDeg) {
    if(!CanPlay() || state_.screen!=Screen::Gameplay || state_.interior.empty()) return Result::Error("Walk position is only valid inside a walkable interior");
    if(xCm<-800 || xCm>800 || yCm<-800 || yCm>800 || zCm<0 || zCm>250 || facingDeg<-180 || facingDeg>180) return Result::Error("Walk position outside the Garage volume");
    state_.walk={xCm,yCm,zCm,facingDeg};return Result::Success();
}
std::optional<InteriorPoint> Campaign::NearbyPoint(int rangeCm) const {
    if(state_.interior.empty() || rangeCm<1) return std::nullopt;
    const auto profiles=InteriorProfiles();
    const auto it=std::find_if(profiles.begin(),profiles.end(),[&](const InteriorProfile& p){return p.id==state_.interior;});
    if(it==profiles.end()) return std::nullopt;
    std::optional<InteriorPoint> best;std::int64_t bestD=-1;
    for(const auto& p:it->points) {
        const int dx=state_.walk.xCm-p.xCm,dy=state_.walk.yCm-p.yCm,dz=state_.walk.zCm-90;
        if(!WithinInteractionRange(dx,dy,dz,rangeCm)) continue;
        const auto d=static_cast<std::int64_t>(dx)*dx+static_cast<std::int64_t>(dy)*dy+static_cast<std::int64_t>(dz)*dz;
        if(!best || d<bestD) {best=p;bestD=d;}
    }
    return best;
}
Result Campaign::InteractNearby(int rangeCm) {
    if(!CanPlay() || state_.screen!=Screen::Gameplay) return Result::Error("Interaction requires Garage walk mode");
    const auto point=NearbyPoint(rangeCm);if(!point) return Result::Error("Nothing in interaction range");
    if(point->action=="city") return ReturnToCity();
    if(point->action=="talk") return TalkToGarageNpc();
    if(point->action=="review") {Visit(Screen::Training);return Result::Success("Review desk");}
    if(point->action=="warehouse") return Result::Success("Warehouse terminal");
    if(point->action=="location") return Result::Success("Location terminal");
    return Result::Error("Unknown interaction");
}
} // namespace mai
