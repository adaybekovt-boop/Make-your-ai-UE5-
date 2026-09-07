#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/MaiDomain.h"
#include "Gameplay/MaiCatalogAsset.h"
#include "Persistence/MaiSaveGame.h"
#include "Persistence/MaiSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiCatalogAutomation, "MakeYourAI.Catalog.DefaultAssetParity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiCatalogAutomation::RunTest(const FString& Parameters) {
    (void)Parameters; auto* Asset = NewObject<UMaiCatalogAsset>(); mai::Catalog Catalog; FString Error;
    TestTrue(TEXT("DataAsset converts"), Asset->ToDomain(Catalog, Error));
    TestEqual(TEXT("DataAsset keeps source defaults"), Catalog.Fingerprint(), mai::Catalog::Defaults().Fingerprint());
    TestFalse(TEXT("No invented nuclear balance"), Catalog.extensions.nuclearConfigured);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiGarageAutomation, "MakeYourAI.Garage.PurchaseDeliveryMountPause", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiGarageAutomation::RunTest(const FString& Parameters) {
    (void)Parameters; mai::Simulation S(mai::Catalog::Defaults(), 42);
    TestTrue(TEXT("Buy"), S.BuyLocation("garage").ok);
    TestTrue(TEXT("Order"), S.OrderKit("garage", 0, 0, mai::Channel::Official, 1, 0).ok);
    S.SetPaused(true); const auto Paused = S.Save(); S.AdvanceReal(6 * mai::Hour);
    TestTrue(TEXT("Pause freezes state"), S.Save() == Paused); S.SetPaused(false);
    TestTrue(TEXT("Delivery"), S.AdvanceReal(6 * mai::Hour).ok);
    TestEqual(TEXT("Cash including rent"), static_cast<int64>(S.View().cash), static_cast<int64>(mai::Dollars(5220)));
    TestTrue(TEXT("Mount rack"), S.MountChassis("garage", 0, 0).ok);
    TestTrue(TEXT("Mount chip"), S.MountChip("garage", 0, 0).ok);
    TestEqual(TEXT("No second purchase"), static_cast<int64>(S.View().cash), static_cast<int64>(mai::Dollars(5220)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiSaveAutomation, "MakeYourAI.Persistence.RealUSaveGameRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiSaveAutomation::RunTest(const FString& Parameters) {
    (void)Parameters; mai::Simulation S; S.BuyLocation("garage"); S.OrderKit("garage", 0, 0, mai::Channel::Grey, 1, 0);
    const std::string Before = S.Save();
    auto* Save = NewObject<UMaiSaveGame>(); Save->DomainPayload.Append(reinterpret_cast<const uint8*>(Before.data()), static_cast<int32>(Before.size()));
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Serialize via Unreal"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
    auto* Restored = Cast<UMaiSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("USaveGame class restored"), Restored)) return false;
    TestTrue(TEXT("Payload retained"), Restored->DomainPayload == Save->DomainPayload);
    mai::Simulation Reload;
    TestTrue(TEXT("Domain reload"), Reload.Load(std::string(reinterpret_cast<const char*>(Restored->DomainPayload.GetData()), static_cast<std::size_t>(Restored->DomainPayload.Num()))).ok);
    TestTrue(TEXT("All pending state retained"), Reload.Save() == Before);
    const FString Slot = TEXT("MAI_Automation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TestTrue(TEXT("Disk save"), UGameplayStatics::SaveGameToSlot(Save, Slot, 0));
    auto* Disk = Cast<UMaiSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    TestTrue(TEXT("Disk payload"), Disk && Disk->DomainPayload == Save->DomainPayload);
    TestTrue(TEXT("Remove only this test-created slot"), UGameplayStatics::DeleteGameInSlot(Slot, 0));
    TestFalse(TEXT("Reject path traversal"), UMaiSaveSubsystem::IsSafeSlotName(TEXT("../other")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiNpcAutomation, "MakeYourAI.NPC.ProximityStateMachine", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiNpcAutomation::RunTest(const FString& Parameters) {
    (void)Parameters; mai::NpcState S;
    TestTrue(TEXT("First entry"), mai::UpdateProximity(S, true, 0, 10, 40));
    TestFalse(TEXT("No repeat while inside"), mai::UpdateProximity(S, true, 1, 10, 40));
    mai::UpdateProximity(S, true, 10, 10, 40);
    TestTrue(TEXT("React stage"), S.mode == mai::NpcMode::React);
    mai::UpdateProximity(S, false, 41, 10, 40);
    TestTrue(TEXT("Re-entry after cooldown"), mai::UpdateProximity(S, true, 42, 10, 40)); return true;
}
#endif
