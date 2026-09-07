#include "World/MaiCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AMaiCameraPawn::AMaiCameraPawn() {
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("GroundAnchor"));
    Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm")); Arm->SetupAttachment(RootComponent);
    Arm->TargetArmLength = 50000.f; Arm->SetRelativeRotation(FRotator(-50.f, 45.f, 0)); Arm->bDoCollisionTest = false;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("StrategyCamera")); Camera->SetupAttachment(Arm);
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic; Camera->OrthoWidth = 48000.f;
}
void AMaiCameraPawn::SetupPlayerInputComponent(UInputComponent* Input) {
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MaiNorth"), this, &AMaiCameraPawn::MoveNorth);
    Input->BindAxis(TEXT("MaiEast"), this, &AMaiCameraPawn::MoveEast);
    Input->BindAction(TEXT("MaiZoomIn"), IE_Pressed, this, &AMaiCameraPawn::ZoomIn);
    Input->BindAction(TEXT("MaiZoomOut"), IE_Pressed, this, &AMaiCameraPawn::ZoomOut);
}
void AMaiCameraPawn::MoveNorth(float Value) {
    if (GetWorld() && !FMath::IsNearlyZero(Value)) AddActorWorldOffset(FVector(Value * Camera->OrthoWidth * 0.5f * GetWorld()->GetDeltaSeconds(), 0, 0));
}
void AMaiCameraPawn::MoveEast(float Value) {
    if (GetWorld() && !FMath::IsNearlyZero(Value)) AddActorWorldOffset(FVector(0, Value * Camera->OrthoWidth * 0.5f * GetWorld()->GetDeltaSeconds(), 0));
}
void AMaiCameraPawn::Focus(const FVector& Position, float Width) { SetActorLocation(Position); Camera->OrthoWidth = FMath::Clamp(Width, 1200.f, 90000.f); }
void AMaiCameraPawn::ZoomIn() { Camera->OrthoWidth = FMath::Max(1200.f, Camera->OrthoWidth / 1.2f); }
void AMaiCameraPawn::ZoomOut() { Camera->OrthoWidth = FMath::Min(90000.f, Camera->OrthoWidth * 1.2f); }
