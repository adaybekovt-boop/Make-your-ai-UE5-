#include "World/MaiPlayerController.h"
#include "World/MaiCameraPawn.h"
#include "World/MaiScaffoldWorld.h"
#include "NPC/MaiDemoNpc.h"
#include "Interaction/MaiInteractable.h"
#include "UI/MaiHUDWidget.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void AMaiPlayerController::BeginPlay() {
    Super::BeginPlay(); if (!IsLocalController()) return;
    bShowMouseCursor = true; bEnableClickEvents = true;
    Screen = CreateWidget<UMaiHUDWidget>(this, UMaiHUDWidget::StaticClass()); if (Screen) Screen->AddToViewport();
    FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
}
void AMaiPlayerController::SetupInputComponent() {
    Super::SetupInputComponent(); InputComponent->BindAction(TEXT("MaiSelect"), IE_Pressed, this, &AMaiPlayerController::ClickWorld);
}
void AMaiPlayerController::ClickWorld() {
    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit)) {
        auto* Actor = Hit.GetActor();
        if (Actor && Actor->GetClass()->ImplementsInterface(UMaiInteractable::StaticClass())) IMaiInteractable::Execute_Interact(Actor, this);
    }
}
void AMaiPlayerController::SelectLocation(const FString& Id, int32 Cell) { if (Screen) Screen->SelectLocation(Id, Cell); }
void AMaiPlayerController::ShowCity() { if (auto* Camera = Cast<AMaiCameraPawn>(GetPawn())) Camera->Focus(FVector::ZeroVector, 48000); }
void AMaiPlayerController::FocusLandmark(const FString& Id) {
    if (auto* Camera = Cast<AMaiCameraPawn>(GetPawn())) for (TActorIterator<AMaiScaffoldWorld> It(GetWorld()); It; ++It) { Camera->Focus(It->FindLandmark(Id), 7000); break; }
}
void AMaiPlayerController::EnterLocation(const FString& Id) {
    if (auto* Camera = Cast<AMaiCameraPawn>(GetPawn())) for (TActorIterator<AMaiScaffoldWorld> It(GetWorld()); It; ++It) {
        if (It->Enter(Id)) { Camera->Focus(It->InteriorOrigin(), Id == TEXT("garage") ? 2200 : 4500); if (Screen) Screen->SelectLocation(Id); } break;
    }
}
void AMaiPlayerController::VisitNpc() {
    if (auto* Camera = Cast<AMaiCameraPawn>(GetPawn())) for (TActorIterator<AMaiDemoNpc> It(GetWorld()); It; ++It) { Camera->Focus(It->GetActorLocation() + FVector(100, 0, 0), 2000); break; }
}
