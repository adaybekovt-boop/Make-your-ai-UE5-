#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Misc/ScopeExit.h"
#include "Campaign/MaiCampaign.h"
#include "Campaign/MaiCampaignAsset.h"
#include "Core/MaiStrings.h"
#include "Persistence/MaiSaveGame.h"
#include "World/MaiGarageInterior.h"
#include "World/MaiWalkCharacter.h"
#include "World/MaiCityLighting.h"
#include "Interaction/MaiInteriorPoint.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
namespace {
bool Prepare(mai::Campaign& G) {
    return G.CompleteLoad(G.View().loading.generation,true).ok && G.BeginNewGame().ok && G.ShowDifficulty().ok &&
        G.ChooseDifficulty("Normal").ok && G.PrologueAction("UE Automation company").ok && G.PrologueAction("budget").ok &&
        G.PrologueAction("accept-task").ok && G.CompleteLoad(G.View().loading.generation,true).ok &&
        G.Core().BuyLocation("garage").ok && G.Core().OrderKit("garage",0,0,mai::Channel::Official,1,0).ok &&
        G.AdvanceReal(6*mai::Hour).ok && G.Core().MountChassis("garage",0,0).ok && G.Core().MountChip("garage",0,0).ok &&
        G.BeginLoad(mai::Screen::Gameplay,"garage").ok && G.CompleteLoad(G.View().loading.generation,true).ok;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiCampaignAssetAutomation,"MakeYourAI.Campaign.EditableProfilesMatchDomain",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMaiCampaignAssetAutomation::RunTest(const FString& Parameters) {
    (void)Parameters;auto* Asset=NewObject<UMaiCampaignAsset>();mai::CampaignRules Rules;FString Error;
    TestEqual(TEXT("Windows never emit in full daylight"),AMaiCityLighting::NightWindowStrength(.65f,1.f),0.f);
    TestEqual(TEXT("Non-emissive facades remain non-emissive"),AMaiCityLighting::NightWindowStrength(0.f,0.f),0.f);
    TestTrue(TEXT("Weak authored evening windows remain visible at night"),AMaiCityLighting::NightWindowStrength(.03f,0.f)>=.8f);
    for(float Lux : {.4f, 1.f, 50.f, 500.f, 3000.f, 6000.f, 12000.f})
        TestTrue(TEXT("Exposure compensates sun intensity throughout twilight"),
            FMath::IsNearlyEqual(Lux/FMath::Pow(2.f,AMaiCityLighting::ExposureForSunLux(Lux)),2.5f,.001f));
    if(!TestTrue(TEXT("DataAsset converts"),Asset->ToDomain(Rules,Error)))return false;
    TestEqual(TEXT("All editable defaults preserved"),Rules.Fingerprint(),mai::CampaignRules::Defaults().Fingerprint());
    TestEqual(TEXT("Three difficulties"),Asset->Difficulties.Num(),3);TestEqual(TEXT("Five ending profiles"),Asset->Endings.Num(),5);
    TestTrue(TEXT("Buyer is fictional"),Rules.buyer.fictional);TestTrue(TEXT("Missing portrait is not silently substituted"),Asset->ElonMaxPortrait.IsNull());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiCampaignSaveAutomation,"MakeYourAI.Campaign.FullFlowRealSaveGame",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMaiCampaignSaveAutomation::RunTest(const FString& Parameters) {
    (void)Parameters;mai::Campaign G;if(!TestTrue(TEXT("Garage setup"),Prepare(G)))return false;
    TestTrue(TEXT("Dataset purchase"),G.BuyDataset("official-image-10").ok);TestTrue(TEXT("Manual image review"),G.StartReview(1,mai::ReviewMethod::Manual).ok);
    while(G.Review(1) && G.Review(1)->decisions.size()<G.Review(1)->items.size()){
        const auto* R=G.Review(1);if(!TestTrue(TEXT("Real paired decision"),G.ChooseReview(1,R->items[R->decisions.size()].betterSide).ok))return false;
    }
    TestTrue(TEXT("Start training"),G.StartTraining(1).ok);TestTrue(TEXT("Complete quality-weighted work"),G.AdvanceReal(5*mai::Hour).ok);
    const std::string Before=G.Save();auto* Save=NewObject<UMaiSaveGame>();Save->Difficulty=TEXT("Normal");
    Save->CurrentScreen=static_cast<int32>(G.View().screen);Save->FirstScreen=static_cast<int32>(G.View().firstScreen);Save->LastScreen=static_cast<int32>(G.View().lastScreen);
    Save->DomainPayload.Append(reinterpret_cast<const uint8*>(Before.data()),static_cast<int32>(Before.size()));
    const FString Slot=TEXT("MAI_Campaign_Automation_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(Slot,0); };
    if(!TestTrue(TEXT("Actual USaveGame disk write"),UGameplayStatics::SaveGameToSlot(Save,Slot,0)))return false;
    auto* Disk=Cast<UMaiSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!TestNotNull(TEXT("Disk class"),Disk))return false;
    TestEqual(TEXT("Outer version"),Disk->FormatVersion,2);TestEqual(TEXT("Saved difficulty"),Disk->Difficulty,FString(TEXT("Normal")));
    mai::Campaign Reload;TestTrue(TEXT("Reconstructed campaign"),Reload.Load(std::string(reinterpret_cast<const char*>(Disk->DomainPayload.GetData()),Disk->DomainPayload.Num())).ok);
    TestTrue(TEXT("All state roundtrips"),Reload.Save()==Before);TestTrue(TEXT("Ending evaluation"),Reload.EvaluateEnding().ok);
    TestTrue(TEXT("Normal is not a fabricated global victory"),Reload.View().ending.kind==mai::EndingKind::None);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiCampaignGaragePhysicsAutomation,"MakeYourAI.Campaign.GarageSweptMovementAndInteraction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMaiCampaignGaragePhysicsAutomation::RunTest(const FString& Parameters) {
    (void)Parameters;LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    if(!TestNotNull(TEXT("Isolated real engine world"),World))return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false);GEngine->DestroyWorldContext(World); };
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Room=World->SpawnActor<AMaiGarageInterior>(FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
    if(!TestTrue(TEXT("Authored room and collision built"),Room && Room->Build()))return false;
    auto* Character=World->SpawnActor<AMaiWalkCharacter>(FVector(-175,-180,100),FRotator::ZeroRotator,Spawn);
    if(!TestNotNull(TEXT("ACharacter with movement component"),Character))return false;
    AMaiInteriorPoint* Desk=nullptr;for(TActorIterator<AMaiInteriorPoint> It(World);It;++It)if(It->Action==TEXT("review"))Desk=*It;
    if(!TestNotNull(TEXT("Real review point"),Desk))return false;
    TestTrue(TEXT("Near desk permits trace-based interaction"),Desk->CanInteract(Character));
    FHitResult Hit;Character->GetCharacterMovement()->SafeMoveUpdatedComponent(FVector(0,50,0),FQuat::Identity,true,Hit);
    TestTrue(TEXT("Movement component changes physical location"),Character->GetActorLocation().Y>-135);
    Character->GetCharacterMovement()->SafeMoveUpdatedComponent(FVector(0,-1200,0),FQuat::Identity,true,Hit);
    TestTrue(TEXT("Wall stops swept capsule"),Hit.bBlockingHit);TestTrue(TEXT("Character stays inside room"),Character->GetActorLocation().Y>-710);
    Character->SetActorLocation(Room->PlayerStart(),false,nullptr,ETeleportType::TeleportPhysics);
    TestFalse(TEXT("Review desk cannot be activated from the entrance"),Desk->CanInteract(Character));
    for(const auto& Profile:mai::InteriorProfiles()) {
        Room->ConfigureLocation(UTF8_TO_TCHAR(Profile.id.c_str()));
        TestTrue(TEXT("Server room builds"),Room->Build());
        Character->SetActorLocation(Room->PlayerStart(),false,nullptr,ETeleportType::TeleportPhysics);
        Character->SetRoomBounds(Room->GetActorLocation(),Room->WalkHalfSize());
        const FVector Start=Character->GetActorLocation();
        for(TActorIterator<AMaiInteriorPoint> It(World);It;++It)if(It->Cell>=0)
            TestTrue(TEXT("Spawn aisle clears rack columns including occupied rear row"),
                FMath::Abs(Start.X-It->GetActorLocation().X)>72.f);
        for(const FVector Escape:{FVector(10000,0,100),FVector(0,10000,100),FVector(0,0,-500)}) {
            Character->SetActorLocation(Escape,false,nullptr,ETeleportType::TeleportPhysics);Character->Tick(0);
            TestTrue(TEXT("Escaped capsule returns to safe spawn"),Character->GetActorLocation().Equals(Start,1));
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiCampaignFlow, "MakeYourAI.Campaign.LoadingDifficultyAndInventory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiCampaignFlow::RunTest(const FString& Parameters) {
    (void)Parameters;
    mai::Campaign C;
    TestTrue(TEXT("Boot load"), C.View().loading.phase == mai::LoadPhase::Loading);
    TestFalse(TEXT("Boot cannot cancel"), C.CancelLoad().ok);
    TestTrue(TEXT("Complete boot"), C.CompleteLoad(C.View().loading.generation, true).ok);
    TestTrue(TEXT("New game"), C.BeginNewGame().ok);
    TestTrue(TEXT("Difficulty screen"), C.ShowDifficulty().ok);
    TestTrue(TEXT("Normal"), C.ChooseDifficulty("Normal").ok);
    TestEqual(TEXT("Normal cash"), static_cast<int64>(C.Core().View().cash), static_cast<int64>(mai::Dollars(12000)));
    TestFalse(TEXT("Locked"), C.ChooseDifficulty("Easy").ok);
    TestTrue(TEXT("Name"), C.PrologueAction("Neuron").ok);
    TestTrue(TEXT("Budget"), C.PrologueAction("budget").ok);
    TestTrue(TEXT("Task"), C.PrologueAction("accept-task").ok);
    TestTrue(TEXT("City load"), C.CompleteLoad(C.View().loading.generation, true).ok);
    auto Funded = C.Core().View(); Funded.cash = mai::Dollars(100000); C.Core().Restore(Funded);
    TestTrue(TEXT("Buy dataset"), C.BuyDataset("official-text-10").ok);
    TestTrue(TEXT("Unreviewed"), C.Batch(1) && C.Batch(1)->status == mai::DatasetStatus::Unreviewed);
    TestFalse(TEXT("No train yet"), C.StartTraining(1).ok);
    TestTrue(TEXT("Elon Max fictional"), C.Rules().buyer.fictional);
    TestTrue(TEXT("Portrait import still pending"), FString(UTF8_TO_TCHAR(mai::Loc("placeholder.elon-max"))).Contains(TEXT("PLACEHOLDER")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiEndingEvaluator, "MakeYourAI.Campaign.EndingEvaluator", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMaiEndingEvaluator::RunTest(const FString& Parameters) {
    (void)Parameters;
    const auto Rules = mai::CampaignRules::Defaults();
    mai::EndingMetrics None; None.difficulty = "Normal";
    TestTrue(TEXT("No ending"), mai::EndingEvaluator::Evaluate(None, Rules.endings).kind == mai::EndingKind::None);
    mai::EndingMetrics Bankrupt = None; Bankrupt.bankrupt = true;
    TestTrue(TEXT("Bankruptcy"), mai::EndingEvaluator::Evaluate(Bankrupt, Rules.endings).kind == mai::EndingKind::Bankruptcy);
    mai::EndingMetrics Regulator = None; Regulator.legalBps = 9000; Regulator.ignoredWarnings = 3;
    TestTrue(TEXT("Regulator beats bankruptcy"), mai::EndingEvaluator::Evaluate(Regulator, Rules.endings).kind == mai::EndingKind::Regulator);
    return true;
}
#endif
