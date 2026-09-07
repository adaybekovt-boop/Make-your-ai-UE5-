#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Misc/ScopeExit.h"
#include "Campaign/MaiCampaign.h"
#include "Campaign/MaiCampaignAsset.h"
#include "Persistence/MaiSaveGame.h"
#include "World/MaiGarageInterior.h"
#include "World/MaiWalkCharacter.h"
#include "Interaction/MaiInteriorPoint.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
namespace {
bool Prepare(mai::Campaign& G) {
    return G.CompleteLoad(G.View().loading.generation,true).ok && G.BeginNewGame().ok && G.ShowDifficulty().ok &&
        G.ChooseDifficulty("Standard").ok && G.PrologueAction("UE Automation company").ok && G.PrologueAction("budget").ok &&
        G.PrologueAction("accept-task").ok && G.CompleteLoad(G.View().loading.generation,true).ok &&
        G.Core().BuyLocation("garage").ok && G.Core().OrderKit("garage",0,0,mai::Channel::Official,1,0).ok &&
        G.AdvanceReal(6*mai::Hour).ok && G.Core().MountChassis("garage",0,0).ok && G.Core().MountChip("garage",0,0).ok &&
        G.BeginLoad(mai::Screen::Gameplay,"garage").ok && G.CompleteLoad(G.View().loading.generation,true).ok;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiCampaignAssetAutomation,"MakeYourAI.Campaign.EditableProfilesMatchDomain",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMaiCampaignAssetAutomation::RunTest(const FString& Parameters) {
    (void)Parameters;auto* Asset=NewObject<UMaiCampaignAsset>();mai::CampaignRules Rules;FString Error;
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
    const std::string Before=G.Save();auto* Save=NewObject<UMaiSaveGame>();Save->Difficulty=TEXT("Standard");
    Save->CurrentScreen=static_cast<int32>(G.View().screen);Save->FirstScreen=static_cast<int32>(G.View().firstScreen);Save->LastScreen=static_cast<int32>(G.View().lastScreen);
    Save->DomainPayload.Append(reinterpret_cast<const uint8*>(Before.data()),static_cast<int32>(Before.size()));
    const FString Slot=TEXT("MAI_Campaign_Automation_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(Slot,0); };
    if(!TestTrue(TEXT("Actual USaveGame disk write"),UGameplayStatics::SaveGameToSlot(Save,Slot,0)))return false;
    auto* Disk=Cast<UMaiSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!TestNotNull(TEXT("Disk class"),Disk))return false;
    TestEqual(TEXT("Outer version"),Disk->FormatVersion,2);TestEqual(TEXT("Saved difficulty"),Disk->Difficulty,FString(TEXT("Standard")));
    mai::Campaign Reload;TestTrue(TEXT("Reconstructed campaign"),Reload.Load(std::string(reinterpret_cast<const char*>(Disk->DomainPayload.GetData()),Disk->DomainPayload.Num())).ok);
    TestTrue(TEXT("All state roundtrips"),Reload.Save()==Before);TestTrue(TEXT("Ending evaluation"),Reload.EvaluateEnding().ok);
    TestTrue(TEXT("Startup is not a fabricated global victory"),Reload.View().ending.kind==mai::EndingKind::None);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaiCampaignGaragePhysicsAutomation,"MakeYourAI.Campaign.GarageSweptMovementAndInteraction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMaiCampaignGaragePhysicsAutomation::RunTest(const FString& Parameters) {
    (void)Parameters;LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    if(!TestNotNull(TEXT("Isolated real engine world"),World))return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Room=World->SpawnActor<AMaiGarageInterior>(FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
    if(!TestTrue(TEXT("Real collision graybox built"),Room && Room->Build()))return false;
    auto* Character=World->SpawnActor<AMaiWalkCharacter>(FVector(-400,120,100),FRotator::ZeroRotator,Spawn);
    if(!TestNotNull(TEXT("ACharacter with movement component"),Character))return false;
    AMaiInteriorPoint* Desk=nullptr;for(TActorIterator<AMaiInteriorPoint> It(World);It;++It)if(It->Action==TEXT("review"))Desk=*It;
    if(!TestNotNull(TEXT("Real review point"),Desk))return false;
    TestTrue(TEXT("Near desk permits trace-based interaction"),Desk->CanInteract(Character));
    FHitResult Hit;Character->GetCharacterMovement()->SafeMoveUpdatedComponent(FVector(0,50,0),FQuat::Identity,true,Hit);
    TestTrue(TEXT("Movement component changes physical location"),Character->GetActorLocation().Y>160);
    Character->GetCharacterMovement()->SafeMoveUpdatedComponent(FVector(0,-1200,0),FQuat::Identity,true,Hit);
    TestTrue(TEXT("Wall stops swept capsule"),Hit.bBlockingHit);TestTrue(TEXT("Character stays inside room"),Character->GetActorLocation().Y>-710);
    TestFalse(TEXT("Distant review point is not callable"),Desk->CanInteract(Character));return true;
}
#endif
