#include "World/MaiPlayerController.h"
#include "World/MaiCameraPawn.h"
#include "World/MaiWalkPawn.h"
#include "World/MaiScaffoldWorld.h"
#include "World/MaiWalkCharacter.h"
#include "World/MaiGarageInterior.h"
#include "Interaction/MaiInteriorPoint.h"
#include "Interaction/MaiInteractable.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "UI/MaiNativeWidget.h"
#include "Rules/MaiRulesSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UI/MaiHUDWidget.h"
#include "UI/MaiFlowWidget.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "World/MaiCityBatch.h"
#include "World/MaiServerAmbience.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

void AMaiPlayerController::BeginPlay() {
    Super::BeginPlay(); if (!IsLocalController()) return;
    ServerAmbience=NewObject<UMaiServerAmbience>(this);ServerAmbience->RegisterComponent();ServerAmbience->Start();
    bShowMouseCursor = true; bEnableClickEvents = true;
    CityCamera = Cast<AMaiCameraPawn>(GetPawn());
    NativeScreen = CreateWidget<UMaiNativeWidget>(this, UMaiNativeWidget::StaticClass());
    if (NativeScreen) NativeScreen->AddToViewport(0);
    FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
}
void AMaiPlayerController::PlayerTick(float DeltaTime) {
    Super::PlayerTick(DeltaTime);
    const bool WalkingView=Cast<AMaiWalkCharacter>(GetPawn())&&NativeScreen&&NativeScreen->IsWalkingView();
    if(!WalkingView)bWalkCursorFreed=false;
    const bool Capture=WalkingView&&!bWalkCursorFreed;
    if(Capture!=bWalkMouseCaptured || bShowMouseCursor==Capture){
        bWalkMouseCaptured=Capture;bShowMouseCursor=!Capture;
        if(Capture){FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);SetInputMode(Mode);}
        else{FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);}
    }
    QualityClock+=DeltaTime;
    if(QualityClock>=.25f){QualityClock=0;
        auto* View=Cast<AMaiCameraPawn>(GetPawn());const double Height=View?View->GetActorLocation().Z:0;
        const bool Far=bDistantView?Height>16000:Height>22000;
        if(!bQualityInitialized){bDistantView=Far;bQualityInitialized=true;
            // Camera distance is a geometry/shadow quality decision, not exposure.
            if(View&&View->Camera)View->Camera->PostProcessSettings.bOverride_AutoExposureBias=false;
            // Keep lighting continuous during zoom. The former altitude switch
            // changed shadows and every instance's LOD together in one frame.
            for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("MAI_CitySun")))It->GetLightComponent()->SetCastShadows(true);
            // Screen-size LOD selection remains automatic, with one stable scale.
            for(TActorIterator<AMaiCityBatch> It(GetWorld());It;++It)if(It->Instances)It->Instances->SetLODDistanceScale(1.65f);
        }
    }
    auto* C = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    const auto* G = C ? C->CampaignDomain() : nullptr;
    if(ServerAmbience){
        float FanLoad=0;
        if(C && C->Domain() && G && G->CanPlay() && Cast<AMaiWalkCharacter>(GetPawn())){
            const int32 Index=C->Domain()->Definitions().LocationIndex(G->View().interior);
            if(Index>=0)for(const auto& Slot:C->Domain()->View().locations[static_cast<size_t>(Index)].slots)
                if(Slot.chassis>=0)FanLoad+=.18f;
        }
        ServerAmbience->SetServerLoad(FanLoad);
    }
    const bool Operations = G && G->CanPlay() && G->View().screen != mai::Screen::Training && bOperationsOpen;
    if (Screen) Screen->SetVisibility(Operations ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
void AMaiPlayerController::SetupInputComponent() {
    Super::SetupInputComponent(); InputComponent->BindAction(TEXT("MaiSelect"), IE_Pressed, this, &AMaiPlayerController::ClickWorld);
    InputComponent->BindKey(EKeys::Tab,IE_Pressed,this,&AMaiPlayerController::ToggleWalkCursor);
}
void AMaiPlayerController::ClickWorld() {
    if(Cast<AMaiWalkCharacter>(GetPawn()) && !bShowMouseCursor)return;
    auto* C = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (!C || !C->CampaignDomain() || !C->CampaignDomain()->CanPlay() || C->CampaignDomain()->View().screen == mai::Screen::Training) return;
    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit)) {
        auto* A = Hit.GetActor();
        if (A && A->GetClass()->ImplementsInterface(UMaiInteractable::StaticClass())) IMaiInteractable::Execute_Interact(A, this);
    }
}
void AMaiPlayerController::SelectLocation(const FString& Id, int32 Cell) {
    if (auto* Rules=GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>()) {
        Rules->Dispatch(TEXT("ui:location"),{MakeShared<FJsonValueString>(Id)});
        if(Cell>=0) {auto* C=GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();const int I=C->Domain()->Definitions().LocationIndex(TCHAR_TO_UTF8(*Id));if(I>=0){const int Cols=C->Domain()->Definitions().locations[I].cols;Rules->Dispatch(TEXT("ui:cell"),{MakeShared<FJsonValueString>(Id),MakeShared<FJsonValueNumber>(Cell/Cols),MakeShared<FJsonValueNumber>(Cell%Cols)});}}
    }
}
void AMaiPlayerController::PossessCity() {
    FString Error; PrepareCampaignScene(TEXT(""), false, Error);
    bShowMouseCursor = true;
}
void AMaiPlayerController::PossessWalk() {
    FString Error; PrepareCampaignScene(TEXT("garage"), false, Error);
    bShowMouseCursor = false;
}
void AMaiPlayerController::ShowCity() {
    auto* C = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (C) C->CampaignTransact([](mai::Campaign& G){ return G.BeginLoad(mai::Screen::CityMap); });
}
void AMaiPlayerController::EnterLocation(const FString& Id) {
    auto* C = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (C) C->CampaignTransact([&](mai::Campaign& G){ return G.BeginLoad(mai::Screen::Gameplay, TCHAR_TO_UTF8(*Id)); });
}
void AMaiPlayerController::FocusLandmark(const FString& Id) {
    if (auto* Camera = Cast<AMaiCameraPawn>(GetPawn())) for (TActorIterator<AMaiScaffoldWorld> It(GetWorld()); It; ++It) { Camera->Focus(It->FindLandmark(Id), 7000); break; }
}
void AMaiPlayerController::VisitNpc() { EnterLocation(TEXT("garage")); }
bool AMaiPlayerController::AtReviewDesk() const {
    const auto* ReviewPawn = Cast<AMaiWalkCharacter>(GetPawn());
    if (!ReviewPawn) return false;
    for (TActorIterator<AMaiInteriorPoint> It(GetWorld()); It; ++It) if (It->Action == TEXT("review") && It->CanInteract(ReviewPawn)) return true;
    return false;
}
void AMaiPlayerController::OpenTraining(bool bFromDesk) {
    if (bFromDesk && !AtReviewDesk()) return;
    auto* C = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (C) C->CampaignTransact([](mai::Campaign& G){ return G.ShowScreen(mai::Screen::Training); });
    if(auto* Rules=GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>()) Rules->Dispatch(bFromDesk?TEXT("host:review"):TEXT("ui:page"),bFromDesk?TArray<TSharedPtr<FJsonValue>>{}:TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(TEXT("training"))});
}
bool AMaiPlayerController::PrepareCampaignScene(const FString& Interior, bool bMenu, FString& Error) {
    if (!GetWorld()) { Error = TEXT("World unavailable"); return false; }
    if (!IsValid(CityCamera)) CityCamera = GetWorld()->SpawnActor<AMaiCameraPawn>();
    if (!CityCamera) { Error = TEXT("City camera spawn failed"); return false; }
    if (Interior.IsEmpty()) {
        if (IsValid(Walker)) { Walker->Destroy(); Walker = nullptr; }
        if (IsValid(WalkPawn)) { WalkPawn->Destroy(); WalkPawn = nullptr; }
        if (IsValid(RuntimeGarage)) { RuntimeGarage->Destroy(); RuntimeGarage = nullptr; }
        Possess(CityCamera); bOperationsOpen = true; bShowMouseCursor=true;
        if (!bMenu) {
            ACameraActor* Authored=nullptr;
            for(TActorIterator<ACameraActor> It(GetWorld());It;++It) if(It->ActorHasTag(TEXT("MAI_CityCamera"))){Authored=*It;break;}
            if(!Authored){Error=TEXT("Камера и карта CityV4 не загружены. Повторите создание контента в Editor.");return false;}
            bQualityInitialized=false;
            CityCamera->Arm->TargetArmLength=0;CityCamera->Arm->SetRelativeRotation(FRotator::ZeroRotator);
            CityCamera->SetActorTransform(Authored->GetActorTransform());
            CityCamera->Camera->SetProjectionMode(Authored->GetCameraComponent()->ProjectionMode);
            CityCamera->Camera->SetOrthoWidth(Authored->GetCameraComponent()->OrthoWidth);
            CityCamera->Camera->SetFieldOfView(Authored->GetCameraComponent()->FieldOfView);
            CityCamera->RememberOverview();
        }
        return GetPawn() == CityCamera;
    }
    AMaiGarageInterior* Room = nullptr;
    for (TActorIterator<AMaiGarageInterior> It(GetWorld()); It; ++It) if (!It->IsHidden()) { Room = *It; break; }
    if (!Room) { RuntimeGarage = GetWorld()->SpawnActor<AMaiGarageInterior>(FVector(100000, 100000, 0), FRotator::ZeroRotator); Room = RuntimeGarage; }
    if(Room)Room->ConfigureLocation(Interior);
    if (!Room || !Room->Build()) { Error = TEXT("Room construction failed; required mesh or collision components unavailable"); return false; }
    if (IsValid(Walker)) Walker->Destroy();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Walker = GetWorld()->SpawnActor<AMaiWalkCharacter>(Room->PlayerStart(), FRotator::ZeroRotator, Params);
    if (!Walker) { Error = TEXT("Garage character spawn failed"); return false; }
    Walker->SetRoomBounds(Room->GetActorLocation(),Room->WalkHalfSize());
    Possess(Walker);SetControlRotation(FRotator(0,-90,0));bShowMouseCursor=true; bOperationsOpen = false; return GetPawn() == Walker;
}
void AMaiPlayerController::ShowWarehouse() {
    auto* Company=GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if(!Company||!Company->CampaignDomain())return;
    const FString Id=UTF8_TO_TCHAR(Company->CampaignDomain()->View().interior.c_str());
    if(!Id.IsEmpty())if(auto* Rules=GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>())Rules->Dispatch(TEXT("ui:procurement"),{MakeShared<FJsonValueString>(Id)});
}
