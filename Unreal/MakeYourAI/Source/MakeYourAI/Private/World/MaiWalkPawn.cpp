#include "World/MaiWalkPawn.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"

AMaiWalkPawn::AMaiWalkPawn() {
    PrimaryActorTick.bCanEverTick = true;
    Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    Capsule->InitCapsuleSize(42.f, 90.f);
    Capsule->SetCollisionProfileName(TEXT("Pawn"));
    RootComponent = Capsule;
    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->UpdatedComponent = Capsule;
    Movement->MaxSpeed = 380.f;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Capsule);
    Camera->SetRelativeLocation(FVector(0, 0, 64));
    ProxyBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProxyBody"));
    ProxyBody->SetupAttachment(Capsule);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CapsuleMesh.Succeeded()) ProxyBody->SetStaticMesh(CapsuleMesh.Object);
    ProxyBody->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.7f));
    ProxyBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProxyBody->SetCastShadow(true);
}
void AMaiWalkPawn::SetupPlayerInputComponent(UInputComponent* Input) {
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MaiNorth"), this, &AMaiWalkPawn::MoveForward);
    Input->BindAxis(TEXT("MaiEast"), this, &AMaiWalkPawn::MoveRight);
    Input->BindAction(TEXT("MaiInteract"), IE_Pressed, this, &AMaiWalkPawn::Interact);
}
void AMaiWalkPawn::MoveForward(float Value) { if (Value != 0 && Movement) AddMovementInput(GetActorForwardVector(), Value); }
void AMaiWalkPawn::MoveRight(float Value) { if (Value != 0 && Movement) AddMovementInput(GetActorRightVector(), Value); }
void AMaiWalkPawn::Interact() {
    auto* Company = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (Company) Company->RunCampaign([](mai::Campaign& C){ return C.InteractNearby(150); });
}
void AMaiWalkPawn::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);
    auto* Company = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (!Company || !Company->Campaign() || Company->Campaign()->View().screen != mai::Screen::Gameplay) return;
    const FVector Location = GetActorLocation();
    // Interior staging origin is 100000,100000,0 cm. Domain walk is local cm.
    const int X = FMath::Clamp(FMath::RoundToInt(Location.X - 100000.f), -800, 800);
    const int Y = FMath::Clamp(FMath::RoundToInt(Location.Y - 100000.f), -800, 800);
    const int Z = FMath::Clamp(FMath::RoundToInt(Location.Z), 0, 250);
    const int Facing = FMath::RoundToInt(GetActorRotation().Yaw);
    Company->Campaign()->SetWalkPosition(X, Y, Z, FMath::Clamp(Facing, -180, 180));
}
