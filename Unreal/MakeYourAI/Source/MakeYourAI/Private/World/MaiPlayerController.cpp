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

void AMaiPlayerController::BeginPlay() {
    Super::BeginPlay(); if (!IsLocalController()) return;
    bShowMouseCursor = true; bEnableClickEvents = true;
    CityCamera = Cast<AMaiCameraPawn>(GetPawn());
    NativeScreen = CreateWidget<UMaiNativeWidget>(this, UMaiNativeWidget::StaticClass());
    if (NativeScreen) NativeScreen->AddToViewport(0);
    FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
}
void AMaiPlayerController::PlayerTick(float DeltaTime) {
    Super::PlayerTick(DeltaTime);
    QualityClock+=DeltaTime;
    if(QualityClock>=.25f){QualityClock=0;
        auto* View=Cast<AMaiCameraPawn>(GetPawn());const double Height=View?View->GetActorLocation().Z:0;
        const bool Far=bDistantView?Height>16000:Height>22000;
        if(Far!=bDistantView){bDistantView=Far;
            if(View&&View->Camera){View->Camera->PostProcessSettings.bOverride_AutoExposureBias=true;View->Camera->PostProcessSettings.AutoExposureBias=Far?-1.2f:0.f;}
            for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("MAI_CitySun")))It->GetLightComponent()->SetCastShadows(!Far);
        }
    }
    auto* C = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    const auto* G = C ? C->CampaignDomain() : nullptr;
    const bool Operations = G && G->CanPlay() && G->View().screen != mai::Screen::Training && bOperationsOpen;
    if (Screen) Screen->SetVisibility(Operations ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
void AMaiPlayerController::SetupInputComponent() {
    Super::SetupInputComponent(); InputComponent->BindAction(TEXT("MaiSelect"), IE_Pressed, this, &AMaiPlayerController::ClickWorld);
}
void AMaiPlayerController::ClickWorld() {
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
            CityCamera->Arm->TargetArmLength=0;CityCamera->Arm->SetRelativeRotation(FRotator::ZeroRotator);
            CityCamera->SetActorTransform(Authored->GetActorTransform());
            CityCamera->Camera->SetProjectionMode(Authored->GetCameraComponent()->ProjectionMode);
            CityCamera->Camera->SetOrthoWidth(Authored->GetCameraComponent()->OrthoWidth);
            CityCamera->Camera->SetFieldOfView(Authored->GetCameraComponent()->FieldOfView);
            CityCamera->RememberOverview();
        }
        return GetPawn() == CityCamera;
    }
    if (Interior != TEXT("garage")) { Error = TEXT("Прогулка по этому интерьеру пока не подключена. Оборудование доступно в панели площадки."); return false; }
    AMaiGarageInterior* Room = nullptr;
    for (TActorIterator<AMaiGarageInterior> It(GetWorld()); It; ++It) if (!It->IsHidden()) { Room = *It; break; }
    if (!Room) { RuntimeGarage = GetWorld()->SpawnActor<AMaiGarageInterior>(FVector(100000, 100000, 0), FRotator::ZeroRotator); Room = RuntimeGarage; }
    if (!Room || !Room->Build()) { Error = TEXT("Garage construction failed; required mesh or collision components unavailable"); return false; }
    if (IsValid(Walker)) Walker->Destroy();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Walker = GetWorld()->SpawnActor<AMaiWalkCharacter>(Room->PlayerStart(), FRotator::ZeroRotator, Params);
    if (!Walker) { Error = TEXT("Garage character spawn failed"); return false; }
    Possess(Walker); bOperationsOpen = false; return GetPawn() == Walker;
}
void AMaiPlayerController::ShowWarehouse() { if(auto* Rules=GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>()) Rules->Dispatch(TEXT("ui:procurement"),{MakeShared<FJsonValueString>(TEXT("garage"))}); }
