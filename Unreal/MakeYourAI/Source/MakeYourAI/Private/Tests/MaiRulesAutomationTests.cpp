#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/MaiDomain.h"

namespace {
mai::Catalog TestExtensionCatalog() {
    auto C = mai::Catalog::Defaults();
    // Deliberately synthetic test values, never enabled in the production catalog.
    auto& X = C.extensions; X.nuclearConfigured = true; X.nuclearPrice = mai::Dollars(1000); X.nuclearTariffBps = 7500;
    X.auctionConfigured = true; X.auctionReserve = mai::Dollars(4800); X.bidIncrement = mai::Dollars(100);
    X.rivalBudget = mai::Dollars(4900); X.rivalInterval = mai::Hour / 2;
    auto& R = C.regions[static_cast<std::size_t>(C.RegionIndex("greenhaven"))];
    R.configured = true; R.unlockPrice = mai::Dollars(100); R.tariffBps = 8000;
    auto& L = C.locations[static_cast<std::size_t>(C.LocationIndex("greenhaven-site"))];
    L.configured = true; L.price = mai::Dollars(1500); L.rent = mai::Dollars(80); L.rows = 3; L.cols = 3; L.powerWatts = 3000;
    return C;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiRuleTests, "MakeYourAI.Rules.PricesStatusesAndAtomicOrders", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiRuleTests::RunTest(const FString& Parameters) {
    (void)Parameters; mai::Simulation S;
    TestTrue(TEXT("Starting money"), S.View().cash == mai::Dollars(12000));
    TestTrue(TEXT("Home sites available"), S.LocationStatus("campus") == mai::Status::Available);
    TestTrue(TEXT("Overseas locked"), S.LocationStatus("overseas-west") == mai::Status::Locked);
    TestTrue(TEXT("Official quote"), mai::Simulation::OrderPrice(mai::Dollars(2000), mai::Channel::Official, 2) == mai::Dollars(6080));
    TestTrue(TEXT("Grey quote and discount cap"), mai::Simulation::OrderPrice(mai::Dollars(2000), mai::Channel::Grey, 24) == mai::Dollars(42240));
    TestTrue(TEXT("Purchase"), S.BuyLocation("garage").ok);
    auto State = S.View(); State.cash = mai::Dollars(3000); S.Restore(State); const auto Before = S.Save();
    TestFalse(TEXT("Cannot partially buy a kit"), S.OrderKit("garage", 0, 0, mai::Channel::Official, 1, 0).ok);
    TestTrue(TEXT("No mutation on rejection"), S.Save() == Before); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiUpgradeTests, "MakeYourAI.Equipment.UpgradeAndDefect", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiUpgradeTests::RunTest(const FString& Parameters) {
    (void)Parameters; mai::Simulation S; S.BuyLocation("garage");
    auto State = S.View(); State.locations[0].chassisStock[0] = {1,0}; State.locations[0].chipStock[0] = {1,0};
    State.locations[0].chipStock[1] = {1,1}; State.equipmentRng = 1; S.Restore(State);
    TestTrue(TEXT("Official rack"), S.MountChassis("garage", 0, 0).ok);
    TestTrue(TEXT("Official chip"), S.MountChip("garage", 0, 0).ok);
    const auto Money = S.View().cash;
    TestTrue(TEXT("Defective upgrade handled"), S.MountChip("garage", 0, 1).ok);
    TestTrue(TEXT("Working old chip retained"), S.View().locations[0].chipStock[0].total == 1);
    TestTrue(TEXT("Chassis retained"), S.View().locations[0].slots[0].chassis == 0);
    TestTrue(TEXT("Defective new chip disposed"), S.View().locations[0].slots[0].chip == -1);
    TestTrue(TEXT("15 percent source disposal price"), S.View().cash == Money - mai::Dollars(900)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiAuctionTests, "MakeYourAI.Extensions.AuctionWinAndLoss", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiAuctionTests::RunTest(const FString& Parameters) {
    (void)Parameters;
    for (bool Win : {false, true}) {
        mai::Simulation S(TestExtensionCatalog(), 42); S.BuyLocation("garage");
        TestTrue(TEXT("Auction opens"), S.StartAuction("garage").ok);
        TestTrue(TEXT("Escrow bid"), S.Bid(mai::Dollars(Win ? 5000 : 4800)).ok);
        const auto Saved = S.Save(); mai::Simulation Reload(TestExtensionCatalog());
        TestTrue(TEXT("Auction save loads"), Reload.Load(Saved).ok);
        TestTrue(TEXT("Auction settles"), Reload.AdvanceReal(2 * mai::Hour).ok);
        TestTrue(TEXT("Correct phase"), Reload.View().auction.phase == (Win ? mai::AuctionPhase::Won : mai::AuctionPhase::Lost));
        TestTrue(TEXT("No stranded escrow"), Reload.View().auction.escrow == 0);
        TestTrue(TEXT("Winner uses ordinary orders"), Reload.View().orders.size() == (Win ? 2u : 0u));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiRegionPowerTests, "MakeYourAI.Extensions.RegionsAndNuclear", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiRegionPowerTests::RunTest(const FString& Parameters) {
    (void)Parameters; mai::Simulation S(TestExtensionCatalog(), 42);
    auto State = S.View(); State.cash = mai::Dollars(1000000); S.Restore(State);
    TestTrue(TEXT("Unlock Greenhaven"), S.UnlockRegion("greenhaven").ok);
    TestTrue(TEXT("Switch actual region state"), S.SwitchRegion("greenhaven").ok);
    S.BuyLocation("greenhaven-site"); S.OrderKit("greenhaven-site", 0, 0, mai::Channel::Official, 1, 0);
    S.AdvanceReal(6 * mai::Hour); S.MountChassis("greenhaven-site", 0, 0); S.MountChip("greenhaven-site", 0, 0);
    TestTrue(TEXT("Regional rate"), S.Economy().electricity == 28800000);
    const auto Revenue = S.Economy().revenue; TestTrue(TEXT("Buy nuclear"), S.BuyNuclear().ok);
    TestTrue(TEXT("Tariff acts on consumption"), S.Economy().electricity == 21600000);
    TestTrue(TEXT("No plant income"), S.Economy().revenue == Revenue);
    TestFalse(TEXT("Only one plant"), S.BuyNuclear().ok);
    State = S.View(); State.dirtyHistory = true; S.Restore(State);
    TestTrue(TEXT("One shared baseline court risk"), S.CourtRiskPpm() == 50000);
    return true;
}
#endif
