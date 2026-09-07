#include "Campaign/MaiLoadingSubsystem.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "World/MaiPlayerController.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Subsystems/SubsystemCollection.h"

void UMaiLoadingSubsystem::Initialize(FSubsystemCollectionBase& C) {
    Super::Initialize(C); C.InitializeDependency<UMaiCompanySubsystem>(); Company=GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if(GEngine) TravelFailureHandle=GEngine->OnTravelFailure().AddUObject(this,&UMaiLoadingSubsystem::TravelFailed);
}
void UMaiLoadingSubsystem::Invalidate() {
    ++Serial; ActiveGeneration=0; bAssetsReady=false; bMapRequested=false; bTravelNeeded=false; bTravelIssued=false; PendingStream=nullptr; TravelPackage.Empty();
    if (Assets) { Assets->CancelHandle(); Assets.Reset(); }
}
void UMaiLoadingSubsystem::Deinitialize() { Invalidate(); if(GEngine) GEngine->OnTravelFailure().Remove(TravelFailureHandle); Company=nullptr; GarageStream=nullptr; Super::Deinitialize(); }
UWorld* UMaiLoadingSubsystem::GetTickableGameObjectWorld() const { return GetGameInstance()?GetGameInstance()->GetWorld():nullptr; }
bool UMaiLoadingSubsystem::IsTickable() const { const auto* W=GetTickableGameObjectWorld(); return !IsTemplate() && W && W->IsGameWorld() && Company && Company->IsReady(); }
void UMaiLoadingSubsystem::Progress(int32 Bps,const FString& Operation) {
    if (Company && Company->CampaignDomain()) { const auto& L=Company->CampaignDomain()->View().loading; if(L.progressBps!=Bps || L.operation!=TCHAR_TO_UTF8(*Operation)) Company->CampaignTransact([&](mai::Campaign& C){return C.ReportLoading(ActiveGeneration,Bps,TCHAR_TO_UTF8(*Operation));}); }
}
void UMaiLoadingSubsystem::Fail(const FString& Error) {
    if (Assets) Assets->CancelHandle();
    if (PendingStream) { PendingStream->SetShouldBeVisible(false); PendingStream->SetShouldBeLoaded(false); PendingStream->SetIsRequestingUnloadAndRemoval(true); }
    if (PendingStream==GarageStream) GarageStream=nullptr; PendingStream=nullptr;
    if (Company) Company->CampaignTransact([&](mai::Campaign& C){return C.CompleteLoad(ActiveGeneration,false,TCHAR_TO_UTF8(*Error));});
    UE_LOG(LogTemp, Error, TEXT("Campaign loading failed: %s"), *Error);
}
void UMaiLoadingSubsystem::Start() {
    Invalidate(); if (!Company || !Company->CampaignDomain()) return;
    ActiveGeneration=Company->CampaignDomain()->View().loading.generation; CompanyGeneration=Company->Generation(); StartedAt=FPlatformTime::Seconds();
    Required={FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")),FSoftObjectPath(TEXT("/Engine/BasicShapes/Capsule.Capsule")),FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"))};
    const auto& L=Company->CampaignDomain()->View().loading;
    TravelPackage=TEXT("/Game/Scaffold/Maps/L_Scaffold_City");
    bTravelNeeded=L.interior.empty() && L.destination!=mai::Screen::MainMenu && !UGameplayStatics::GetCurrentLevelName(GetGameInstance(),true).Contains(TEXT("L_Scaffold_City")) && FPackageName::DoesPackageExist(TravelPackage);
    if(bTravelNeeded) Required.Add(FSoftObjectPath(TravelPackage+TEXT(".L_Scaffold_City")));
    const uint64 RequestSerial=Serial; TWeakObjectPtr<UMaiLoadingSubsystem> WeakThis(this);
    Assets=UAssetManager::GetStreamableManager().RequestAsyncLoad(Required,FStreamableDelegate::CreateLambda([WeakThis,RequestSerial](){
        if (WeakThis.IsValid() && WeakThis->Serial==RequestSerial) WeakThis->bAssetsReady=true;
    }));
    if (!Assets) Fail(TEXT("Asset manager did not create a load request"));
}
void UMaiLoadingSubsystem::Retry() {
    if (!Company || !Company->CampaignDomain()) return;
    const auto L=Company->CampaignDomain()->View().loading;
    if (L.phase==mai::LoadPhase::Failed) Company->CampaignTransact([&](mai::Campaign& C){return C.BeginLoad(L.destination,L.interior);});
}
void UMaiLoadingSubsystem::Tick(float DeltaTime) {
    (void)DeltaTime; if (!Company || !Company->CampaignDomain()) return;
    const auto L=Company->CampaignDomain()->View().loading;
    if (L.phase!=mai::LoadPhase::Loading) return;
    if (ActiveGeneration!=L.generation || CompanyGeneration!=Company->Generation()) Start();
    if (Company->CampaignDomain()->View().loading.phase!=mai::LoadPhase::Loading) return;
    if (FPlatformTime::Seconds()-StartedAt>120.0) { Fail(TEXT("Load timed out after 120 seconds; inspect the asset/streaming log and retry")); return; }
    int32 Loaded=0; for (const auto& P:Required) if (P.ResolveObject()) ++Loaded;
    if (!bAssetsReady) { Progress(Required.Num()?Loaded*10000/Required.Num():-1,TEXT("Required assets (asset count, not whole-scene percentage)")); return; }
    if (Loaded!=Required.Num()) { Fail(TEXT("A required asset completed with no loaded object")); return; }
    auto* PC=Cast<AMaiPlayerController>(UGameplayStatics::GetPlayerController(GetGameInstance(),0)); if (!PC) { Progress(-1,TEXT("Waiting for player controller")); return; }
    // The city may be World Partition. Preload its package asynchronously, then
    // travel into that world; do not stream a partitioned city as a dynamic sublevel.
    if(bTravelNeeded && !UGameplayStatics::GetCurrentLevelName(GetGameInstance(),true).Contains(TEXT("L_Scaffold_City"))) {
        Progress(-1,TEXT("City package loaded; activating world (exact percentage unavailable)"));
        if(!bTravelIssued) {bTravelIssued=true;GarageStream=nullptr;UGameplayStatics::OpenLevel(GetGameInstance(),FName(*TravelPackage));}
        return;
    }
    if (!bMapRequested) {
        bMapRequested=true;
        const bool Garage=L.interior=="garage";
        const FString Package=TEXT("/Game/Scaffold/Maps/L_Campaign_Garage");
        if (Garage && FPackageName::DoesPackageExist(Package)) {
            PendingStream=GarageStream;
            if (!PendingStream) {
                bool Success=false;
                PendingStream=ULevelStreamingDynamic::LoadLevelInstance(GetGameInstance(),Package,FVector(100000,100000,0),FRotator::ZeroRotator,Success);
                if (!Success || !PendingStream) { Fail(TEXT("Garage level streaming request failed")); return; }
                GarageStream=PendingStream;
            }
            PendingStream->SetShouldBeLoaded(true); PendingStream->SetShouldBeVisible(true);
        }
        if (!Garage && GarageStream) GarageStream->SetShouldBeVisible(false);
    }
    if (PendingStream && (!PendingStream->IsLevelLoaded() || !PendingStream->IsLevelVisible())) {
        Progress(-1,TEXT("Streaming level / waiting for visibility (exact percentage unavailable)")); return;
    }
    Progress(-1,PendingStream?TEXT("Connecting loaded scene and player"):TEXT("Building explicit runtime graybox; not an imported CityV4 verification"));
    FString Error;
    if (!PC->PrepareCampaignScene(UTF8_TO_TCHAR(L.interior.c_str()),L.destination==mai::Screen::MainMenu,Error)) { Fail(Error); return; }
    Company->CampaignTransact([&](mai::Campaign& C){return C.CompleteLoad(ActiveGeneration,true);});
    UE_LOG(LogTemp, Display, TEXT("Campaign required loading completed for generation %llu"), static_cast<unsigned long long>(ActiveGeneration));
}

void UMaiLoadingSubsystem::TravelFailed(UWorld* World,ETravelFailure::Type Type,const FString& Error) {
    (void)World;(void)Type;
    if(bTravelIssued && Company && Company->CampaignDomain() && Company->CampaignDomain()->View().loading.phase==mai::LoadPhase::Loading) Fail(TEXT("World activation failed: ")+Error);
}
