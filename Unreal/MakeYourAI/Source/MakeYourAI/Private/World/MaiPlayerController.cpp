#include "World/MaiPlayerController.h"
#include "World/MaiCameraPawn.h"
#include "World/MaiWalkPawn.h"
#include "World/MaiScaffoldWorld.h"
#include "World/MaiWalkCharacter.h"
#include "World/MaiGarageInterior.h"
#include "Interaction/MaiInteriorPoint.h"
#include "Interaction/MaiInteractable.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "UI/MaiHUDWidget.h"
#include "UI/MaiFlowWidget.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void AMaiPlayerController::BeginPlay() {
    Super::BeginPlay(); if (!IsLocalController()) return;
    bShowMouseCursor = true; bEnableClickEvents = true;
    CityCamera = Cast<AMaiCameraPawn>(GetPawn());
    Screen = CreateWidget<UMaiHUDWidget>(this, UMaiHUDWidget::StaticClass()); if (Screen) Screen->AddToViewport(0);
    Flow = CreateWidget<UMaiFlowWidget>(this, UMaiFlowWidget::StaticClass()); if (Flow) Flow->AddToViewport(10);
    FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
}
void AMaiPlayerController::PlayerTick(float DeltaTime) {
    Super::PlayerTick(DeltaTime);
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
void AMaiPlayerController::SelectLocation(const FString& Id, int32 Cell) { bOperationsOpen = true; if (Screen) Screen->SelectLocation(Id, Cell); }
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
    const auto* Character = Cast<AMaiWalkCharacter>(GetPawn());
    if (!Character) return false;
    for (TActorIterator<AMaiInteriorPoint> It(GetWorld()); It; ++It) if (It->Action == TEXT("review") && It->CanInteract(Character)) return true;
    return false;
}
void AMaiPlayerController::OpenTraining(bool bFromDesk) {
    if (bFromDesk && !AtReviewDesk()) return;
    auto* C = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (C) C->CampaignTransact([](mai::Campaign& G){ return G.ShowScreen(mai::Screen::Training); });
}
bool AMaiPlayerController::PrepareCampaignScene(const FString& Interior, bool bMenu, FString& Error) {
    if (!GetWorld()) { Error = TEXT("World unavailable"); return false; }
    if (!IsValid(CityCamera)) CityCamera = GetWorld()->SpawnActor<AMaiCameraPawn>();
    if (!CityCamera) { Error = TEXT("City camera spawn failed"); return false; }
    if (Interior.IsEmpty()) {
        if (IsValid(Walker)) { Walker->Destroy(); Walker = nullptr; }
        if (IsValid(WalkPawn)) { WalkPawn->Destroy(); WalkPawn = nullptr; }
        if (IsValid(RuntimeGarage)) { RuntimeGarage->Destroy(); RuntimeGarage = nullptr; }
        Possess(CityCamera); CityCamera->Focus(FVector::ZeroVector, 48000); bOperationsOpen = true;
        if (!bMenu) {
            bool Found = false;
            for (TActorIterator<AMaiScaffoldWorld> It(GetWorld()); It; ++It) { Found = true; break; }
            if (!Found && !GetWorld()->SpawnActor<AMaiScaffoldWorld>()) { Error = TEXT("City graybox spawn failed"); return false; }
        }
        return GetPawn() == CityCamera;
    }
    if (Interior != TEXT("garage")) { Error = TEXT("Only Garage walking is connected in this scaffold"); return false; }
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
void AMaiPlayerController::ShowWarehouse() { bOperationsOpen = true; if (Screen) Screen->ShowWarehouse(); }
