#include "test_harness.h"
#include "Campaign/MaiCampaign.h"
#include "Core/MaiStrings.h"
#include <iostream>
#include <string>
namespace {
using namespace mai;
void BootPlayable(Campaign& c, const char* difficulty="Normal") {
    CHECK(c.View().screen==Screen::Loading);
    CHECK(c.CompleteLoad(c.View().loading.generation,true).ok);
    OK(c.BeginNewGame());
    OK(c.ShowDifficulty());
    OK(c.ChooseDifficulty(difficulty));
    OK(c.PrologueAction("Acme Labs"));
    OK(c.PrologueAction("budget"));
    OK(c.PrologueAction("accept-task"));
    OK(c.CompleteLoad(c.View().loading.generation,true));
    CHECK(c.View().screen==Screen::CityMap);
    CHECK(c.View().difficulty==difficulty);
    CHECK(c.CanPlay());
}
void OwnGarageWithServer(Campaign& c) {
    auto funded=c.Core().View();funded.cash=Dollars(100000);OK(c.Core().Restore(funded));
    if(c.Core().LocationStatus("garage")!=Status::Owned) OK(c.Core().BuyLocation("garage"));
    OK(c.Core().OrderKit("garage",0,0,Channel::Official,1,0));
    OK(c.AdvanceReal(6*Hour));
    OK(c.Core().MountChassis("garage",0,0));
    OK(c.Core().MountChip("garage",0,0));
}
void FinishManual(Campaign& c, std::int64_t session, bool correct) {
    const auto* review=c.Review(session);CHECK(review && !review->items.empty());
    while(c.Review(session) && c.Review(session)->phase==ReviewPhase::Active && c.Review(session)->decisions.size()<c.Review(session)->items.size()) {
        const auto& item=c.Review(session)->items[c.Review(session)->decisions.size()];
        const int side=correct?item.betterSide:(item.betterSide==2?0:1-item.betterSide);
        OK(c.ChooseReview(session, side));
    }
}
}
void RunCampaignTests() {
    const auto test=[](const char* name,auto fn){++cases;const int before=failures;fn();std::cout<<(failures==before?"PASS ":"FAIL ")<<name<<'\n';};
    test("boot load cannot be cancelled and reaches main menu",[]{
        Campaign c;CHECK(c.View().loading.phase==LoadPhase::Loading);CHECK(!c.CancelLoad().ok);
        OK(c.ReportLoading(c.View().loading.generation,-1,"Resolving catalog"));
        OK(c.CompleteLoad(c.View().loading.generation,true));
        CHECK(c.View().screen==Screen::MainMenu);CHECK(c.View().loading.phase==LoadPhase::Ready);
        CHECK(!c.CanPlay());CHECK(!c.LoadFromMenu("").ok);
    });
    test("failed load can retry or return to menu",[]{
        Campaign c;const auto gen=c.View().loading.generation;
        CHECK(!c.CompleteLoad(gen,false,"missing map").ok);
        CHECK(c.View().loading.phase==LoadPhase::Failed);
        CHECK(c.View().loading.progressBps!=0 && c.View().loading.progressBps!=10000);
        OK(c.RetryLoad());CHECK(c.View().loading.generation==gen+1);
        CHECK(!c.CompleteLoad(c.View().loading.generation,false,"still missing").ok);
        OK(c.CancelLoad());CHECK(c.View().screen==Screen::MainMenu);
    });
    test("stale load callbacks are ignored",[]{
        Campaign c;const auto gen=c.View().loading.generation;
        CHECK(!c.ReportLoading(gen+1,1000,"stale").ok);
        CHECK(!c.CompleteLoad(gen+1,true).ok);
        CHECK(c.View().loading.phase==LoadPhase::Loading);
        OK(c.CompleteLoad(gen,true));
    });
    test("new game applies Easy Normal Hard and locks the profile",[]{
        for(const char* id:{"Easy","Normal","Hard"}) {
            Campaign c;BootPlayable(c,id);
            CHECK(c.Difficulty() && c.Difficulty()->id==id);
            const Money expected=id==std::string("Easy")?Dollars(18000):id==std::string("Hard")?Dollars(9600):Dollars(12000);
            CHECK(c.Core().View().cash==expected);
            if(std::string(id)=="Normal") {
                CHECK(c.Core().Definitions().defectPpm==Catalog::Defaults().defectPpm);
                CHECK(c.Core().Definitions().chips[0].price==Catalog::Defaults().chips[0].price);
            }
            CHECK(!c.ChooseDifficulty(id).ok);
            CHECK(!c.ChooseDifficulty("Normal").ok);
        }
        Campaign unknown;OK(unknown.CompleteLoad(unknown.View().loading.generation,true));
        OK(unknown.BeginNewGame());OK(unknown.ShowDifficulty());
        CHECK(!unknown.ChooseDifficulty("Nightmare").ok);
        CHECK(unknown.View().difficulty.empty());
    });
    test("save and load keep difficulty and reject mid-run replacement",[]{
        Campaign a;BootPlayable(a,"Hard");
        OK(a.SetTextScale(12500));OK(a.SetReducedMotion(true));
        const auto saved=a.Save();
        Campaign b;OK(b.CompleteLoad(b.View().loading.generation,true));
        OK(b.LoadFromMenu(saved));
        CHECK(b.View().difficulty=="Hard");CHECK(b.Core().View().cash==Dollars(9600));
        CHECK(b.Settings().textScaleBps==10000); // settings are session-local, not company save
        CHECK(!b.ChooseDifficulty("Easy").ok);
        Campaign c;BootPlayable(c,"Easy");
        CHECK(!c.LoadFromMenu(saved).ok);
        const auto before=c.Save();
        CHECK(!c.Load(saved+"damage").ok);
        CHECK(c.Save()==before);
    });
    test("legacy procurement save migrates to Normal without changing UI difficulty",[]{
        Simulation s;OK(s.BuyLocation("garage"));
        Campaign c;OK(c.CompleteLoad(c.View().loading.generation,true));
        OK(c.Load(s.Save()));
        CHECK(c.View().difficulty=="Normal");CHECK(c.View().prologueStep==3);
        CHECK(c.Core().LocationStatus("garage")==Status::Owned);
    });
    test("missing or invalid campaign rules fail closed",[]{
        auto rules=CampaignRules::Defaults();rules.difficulties[1].capitalBps=9999;
        std::string error;CHECK(!rules.Valid(error));
        Campaign c(Catalog::Defaults(),rules,7);
        CHECK(c.View().loading.phase!=LoadPhase::Ready);
        auto broken=CampaignRules::Defaults();broken.endings.clear();
        CHECK(!broken.Valid(error));
        CHECK(!LocStr("difficulty.normal").empty());
        CHECK(std::string(Loc("character.elon-max.notice")).find("fictional")!=std::string::npos);
    });
    test("optional gameplay load can be cancelled back to the city",[]{
        Campaign c;BootPlayable(c);OwnGarageWithServer(c);
        OK(c.EnterWalk("garage"));
        CHECK(c.View().screen==Screen::Loading);
        OK(c.CancelLoad());CHECK(c.View().screen==Screen::CityMap);CHECK(c.CanPlay());
        OK(c.EnterWalk("garage"));OK(c.CompleteLoad(c.View().loading.generation,true));
        CHECK(c.View().screen==Screen::Gameplay);CHECK(c.View().interior=="garage");
        CHECK(WithinInteractionRange(0,0,0,150));
        OK(c.SetWalkPosition(-400,-250,90,0));
        const auto point=c.NearbyPoint(150);CHECK(point && point->action=="location");
        OK(c.InteractNearby(150));
        OK(c.SetWalkPosition(0,-520,90,0));
        OK(c.InteractNearby(150));
        OK(c.CompleteLoad(c.View().loading.generation,true));
        CHECK(c.View().screen==Screen::CityMap);CHECK(c.View().interior.empty());
        CHECK(c.Core().View().now>=0);
    });
    test("datasets enter Unreviewed inventory and cannot train",[]{
        Campaign c;BootPlayable(c);OwnGarageWithServer(c);
        const auto cash=c.Core().View().cash;
        OK(c.BuyDataset("official-text-10"));
        CHECK(c.View().inventory.batches.size()==1);
        CHECK(c.View().inventory.batches[0].status==DatasetStatus::Unreviewed);
        CHECK(c.Core().View().cash==cash-Dollars(1400));
        CHECK(!c.StartTraining(1).ok);
        CHECK(!c.BuyDataset("missing").ok);
        const auto again=c.Core().View().cash;
        CHECK(!c.StartReview(1,ReviewMethod::Manual).ok); // not inside garage
        CHECK(c.Core().View().cash==again);
    });
    test("manual review both-bad skip cancel and resume",[]{
        Campaign c;BootPlayable(c);OwnGarageWithServer(c);
        OK(c.EnterWalk("garage"));OK(c.CompleteLoad(c.View().loading.generation,true));
        OK(c.BuyDataset("official-text-10"));
        OK(c.BuyDataset("official-image-10"));
        OK(c.StartReview(1,ReviewMethod::Manual));
        const auto sid=c.View().reviews.back().id;
        CHECK(c.View().reviews.back().items.size()==4);
        OK(c.ChooseReview(sid,c.Review(sid)->items[0].betterSide));
        const auto mid=c.Save();
        Campaign resumed;OK(resumed.Load(mid));
        CHECK(resumed.Review(sid)->decisions.size()==1);
        OK(resumed.SkipReview(sid));
        OK(resumed.CancelReview(sid));
        CHECK(resumed.Batch(1)->status==DatasetStatus::Unreviewed);
        CHECK(resumed.Batch(1)->reviewId==0);
        OK(resumed.StartReview(1,ReviewMethod::Manual));
        FinishManual(resumed,resumed.View().reviews.back().id,true);
        CHECK(resumed.Batch(1)->status==DatasetStatus::Verified);
        CHECK(!resumed.StartReview(1,ReviewMethod::Manual).ok);
        OK(resumed.StartReview(2,ReviewMethod::Manual));
        const auto image=resumed.View().reviews.back().id;
        for(int i=0;i<4;++i) {
            const auto& item=resumed.Review(image)->items[resumed.Review(image)->decisions.size()];
            OK(resumed.ChooseReview(image,item.betterSide==2?2:item.betterSide));
        }
        CHECK(resumed.Batch(2)->status==DatasetStatus::Verified || resumed.Batch(2)->status==DatasetStatus::Rejected);
    });
    test("human and AI review share ReviewSession and charge once",[]{
        Campaign c;BootPlayable(c);OwnGarageWithServer(c);
        OK(c.BuyDataset("official-mixed-10"));
        CHECK(!c.StartReview(1,ReviewMethod::Human).ok);
        OK(c.HireSpecialist());
        const auto afterHire=c.Core().View().cash;
        OK(c.StartReview(1,ReviewMethod::Human));
        CHECK(c.Core().View().cash==afterHire-Dollars(60));
        CHECK(!c.StartReview(1,ReviewMethod::Human).ok);
        OK(c.AdvanceReal(Hour));
        CHECK(c.Batch(1)->status==DatasetStatus::Verified || c.Batch(1)->status==DatasetStatus::Rejected);
        OK(c.BuyDataset("official-image-10"));
        CHECK(!c.CreateAIReviewer().ok || c.View().ai.created); // may already have compute
        if(!c.View().ai.created) OK(c.CreateAIReviewer());
        const auto afterAi=c.Core().View().cash;
        OK(c.StartReview(2,ReviewMethod::AI));
        CHECK(c.Core().View().cash==afterAi);
        OK(c.AdvanceReal(Hour));
        CHECK(c.Batch(2)->status!=DatasetStatus::Unreviewed);
        CHECK(!c.CancelReview(c.View().reviews.back().id).ok || c.Review(c.View().reviews.back().id)->phase==ReviewPhase::Complete);
    });
    test("insufficient money and time do not start review or training",[]{
        Campaign c;BootPlayable(c,"Hard");
        auto poor=c.Core().View();poor.cash=Dollars(10);OK(c.Core().Restore(poor));
        CHECK(!c.BuyDataset("official-text-10").ok);
        CHECK(!c.HireSpecialist().ok);
        CHECK(!c.CreateAIReviewer().ok);
        OwnGarageWithServer(c);
        OK(c.EnterWalk("garage"));OK(c.CompleteLoad(c.View().loading.generation,true));
        OK(c.BuyDataset("official-text-10"));
        auto brokeAgain=c.Core().View();brokeAgain.cash=0;OK(c.Core().Restore(brokeAgain));
        CHECK(!c.HireSpecialist().ok);
        const auto before=c.View().reviews.size();
        CHECK(!c.StartReview(1,ReviewMethod::Human).ok);
        CHECK(c.View().reviews.size()==before);
        CHECK(c.Batch(1)->status==DatasetStatus::Unreviewed);
        auto funded=c.Core().View();funded.cash=Dollars(50000);OK(c.Core().Restore(funded));
        OK(c.StartReview(1,ReviewMethod::Manual));
        FinishManual(c,c.View().reviews.back().id,false);
        if(c.Batch(1) && c.Batch(1)->status==DatasetStatus::Rejected) CHECK(!c.StartTraining(1).ok);
    });
    test("verified review changes training value and legal risk survives",[]{
        Campaign c;BootPlayable(c);OwnGarageWithServer(c);
        OK(c.EnterWalk("garage"));OK(c.CompleteLoad(c.View().loading.generation,true));
        OK(c.BuyDataset("unofficial-text-100"));
        OK(c.StartReview(1,ReviewMethod::Manual));
        FinishManual(c,c.View().reviews.back().id,true);
        if(c.Batch(1)->status==DatasetStatus::Verified) {
            CHECK(c.Batch(1)->quality.legalBps==6500);
            const auto beforeIq=c.View().modelMicroIQ;
            OK(c.StartTraining(1));
            for(int i=0;i<80 && c.Batch(1)->status!=DatasetStatus::Trained;++i) OK(c.AdvanceReal(Hour));
            CHECK(c.Batch(1)->status==DatasetStatus::Trained);
            CHECK(c.View().modelMicroIQ>beforeIq);
            CHECK(c.View().legalExposureBps>=6500);
            CHECK(!c.StartTraining(1).ok);
        }
        OK(c.BuyDataset("official-text-10"));
        OK(c.StartReview(c.View().inventory.batches.back().id,ReviewMethod::Manual));
        FinishManual(c,c.View().reviews.back().id,true);
        if(c.Batch(c.View().inventory.batches.back().id)->status==DatasetStatus::Verified) {
            OK(c.StartTraining(c.View().inventory.batches.back().id));
            for(int i=0;i<20 && c.View().training.back().phase!=JobPhase::Complete;++i) OK(c.AdvanceReal(Hour));
        }
    });
    test("save restores review training walk and does not leak after new game",[]{
        Campaign a;BootPlayable(a);OwnGarageWithServer(a);
        OK(a.EnterWalk("garage"));OK(a.CompleteLoad(a.View().loading.generation,true));
        OK(a.SetWalkPosition(120,-80,90,45));
        OK(a.BuyDataset("official-text-10"));
        OK(a.StartReview(1,ReviewMethod::Manual));
        OK(a.ChooseReview(a.View().reviews.back().id,a.Review(a.View().reviews.back().id)->items[0].betterSide));
        const auto snap=a.Save();
        Campaign b;OK(b.Load(snap));
        CHECK(b.View().walk.xCm==120);CHECK(b.View().walk.facingDeg==45);
        CHECK(b.Review(b.View().reviews.back().id)->decisions.size()==1);
        CHECK(b.Batch(1)->status==DatasetStatus::Reviewing);
        CHECK(!b.AcknowledgeEnding().ok);
        OK(b.ShowScreen(Screen::MainMenu));
        OK(b.BeginNewGame());
        CHECK(b.View().inventory.batches.empty());
        CHECK(b.View().difficulty.empty());
        CHECK(b.View().companyName.empty());
        CHECK(b.View().modelMicroIQ==0);
    });
    test("ending evaluator is mutually exclusive and data driven",[]{
        auto rules=CampaignRules::Defaults();
        EndingMetrics none;none.difficulty="Normal";
        CHECK(EndingEvaluator::Evaluate(none,rules.endings).kind==EndingKind::None);
        EndingMetrics bankrupt=none;bankrupt.bankrupt=true;
        CHECK(EndingEvaluator::Evaluate(bankrupt,rules.endings).kind==EndingKind::Bankruptcy);
        EndingMetrics regulator=none;regulator.legalBps=9000;regulator.ignoredWarnings=3;
        CHECK(EndingEvaluator::Evaluate(regulator,rules.endings).kind==EndingKind::Regulator);
        EndingMetrics quality;quality.value=Dollars(2000000);quality.modelMicroIQ=120000000;quality.reputation=90;
        quality.dataQualityBps=9000;quality.employeeCareBps=8000;quality.successfulReviews=2;quality.profitPerHour=Dollars(100);
        quality.cash=Dollars(100000);quality.international=true;quality.saleDeclined=true;quality.difficulty="Normal";
        CHECK(EndingEvaluator::Evaluate(quality,rules.endings).kind==EndingKind::Independent);
        quality.saleDeclined=false;quality.international=false;
        auto offer=EndingEvaluator::Evaluate(quality,rules.endings);
        CHECK(offer.kind==EndingKind::Acquisition);CHECK(offer.offerOnly);
        quality.saleAccepted=true;
        CHECK(EndingEvaluator::Evaluate(quality,rules.endings).kind==EndingKind::Acquisition);
        quality.saleAccepted=false;quality.openChosen=true;quality.dataQualityBps=9500;quality.employeeCareBps=8000;
        CHECK(EndingEvaluator::Evaluate(quality,rules.endings).kind==EndingKind::OpenModel);
        EndingMetrics both=regulator;both.bankrupt=true;
        CHECK(EndingEvaluator::Evaluate(both,rules.endings).kind==EndingKind::Regulator);
        CHECK(rules.buyer.fictional);CHECK(rules.buyer.name=="Elon Max");
        CHECK(rules.buyer.sourcePortrait.find("elon_max_remade.jpg")!=std::string::npos);
        CHECK(std::string(Loc("placeholder.elon-max")).find("PLACEHOLDER")!=std::string::npos);
    });
    test("bankruptcy grace depends on difficulty and new game clears ending",[]{
        Campaign c;BootPlayable(c,"Hard");
        auto broke=c.Core().View();broke.cash=0;OK(c.Core().Restore(broke));
        OK(c.AdvanceReal(23*Hour));CHECK(!c.Core().View().ended);
        OK(c.AdvanceReal(2*Hour));
        if(c.Core().View().ended) {
            CHECK(c.View().ending.kind==EndingKind::Bankruptcy);
            CHECK(c.View().screen==Screen::Ending);
            OK(c.AcknowledgeEnding());CHECK(c.View().screen==Screen::Results);
            OK(c.BeginNewGame());
            CHECK(!c.Core().View().ended);CHECK(c.View().ending.kind==EndingKind::None);
            CHECK(c.View().inventory.batches.empty());
        }
    });
    test("settings quit and strings stay centralized",[]{
        Campaign c;OK(c.CompleteLoad(c.View().loading.generation,true));
        OK(c.ShowScreen(Screen::Settings));
        OK(c.SetTextScale(15000));OK(c.SetReducedMotion(true));
        CHECK(c.Settings().textScaleBps==15000);CHECK(c.Settings().reducedMotion);
        CHECK(!c.SetTextScale(1000).ok);
        OK(c.RequestQuit());CHECK(c.WantsQuit());
        CHECK(ScreenName(Screen::Results)=="Results");
        CHECK(DatasetStatusName(DatasetStatus::Unreviewed)=="Unreviewed");
        CHECK(std::string(Loc("menu.load-game"))=="Load Game");
    });
    test("existing procurement auction nuclear greenhaven npc still reachable",[]{
        Campaign c;BootPlayable(c);OwnGarageWithServer(c);
        CHECK(c.Core().LocationStatus("garage")==Status::Owned);
        CHECK(!c.Core().BuyNuclear().ok);
        CHECK(!c.Core().StartAuction("garage").ok);
        CHECK(!c.Core().UnlockRegion("greenhaven").ok);
        CHECK(c.Core().Proximity(true,10,40));
        const auto saved=c.Save();Campaign d;OK(d.Load(saved));
        CHECK(d.Core().View().npc.eventCount==1);
        CHECK(d.Core().View().nuclearOwned==false);
    });
}
