#include "World/MaiWalkCharacter.h"
#include "Interaction/MaiInteriorPoint.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
AMaiWalkCharacter::AMaiWalkCharacter() {
    GetCapsuleComponent()->InitCapsuleSize(34.f,88.f); GetCharacterMovement()->MaxWalkSpeed=220.f;
    GetCharacterMovement()->bOrientRotationToMovement=true; bUseControllerRotationYaw=false;
    GrayboxBody=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExplicitGrayboxAvatar")); GrayboxBody->SetupAttachment(GetRootComponent());
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Shape.Succeeded()) GrayboxBody->SetStaticMesh(Shape.Object);
    GrayboxBody->SetRelativeScale3D(FVector(.55,.55,1.1)); GrayboxBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Arm=CreateDefaultSubobject<USpringArmComponent>(TEXT("InteriorCameraArm")); Arm->SetupAttachment(GetRootComponent()); Arm->TargetArmLength=650.f;
    Arm->SetUsingAbsoluteRotation(true); Arm->SetRelativeRotation(FRotator(-45,-45,0)); Arm->bUsePawnControlRotation=false;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("InteriorCamera")); Camera->SetupAttachment(Arm);
}
bool AMaiWalkCharacter::CanMoveInCampaign() const {
    auto* C=GetGameInstance()?GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>():nullptr;
    return C && C->CampaignDomain() && C->CampaignDomain()->CanPlay() && C->CampaignDomain()->View().screen==mai::Screen::Gameplay && C->CampaignDomain()->View().interior=="garage";
}
void AMaiWalkCharacter::SetupPlayerInputComponent(UInputComponent* Input) {
    Super::SetupPlayerInputComponent(Input); Input->BindAxis(TEXT("MaiNorth"),this,&AMaiWalkCharacter::North); Input->BindAxis(TEXT("MaiEast"),this,&AMaiWalkCharacter::East);
    Input->BindAction(TEXT("MaiInteract"),IE_Pressed,this,&AMaiWalkCharacter::InteractNearest);
}
void AMaiWalkCharacter::North(float Value) { if (CanMoveInCampaign()) AddMovementInput(FVector::ForwardVector,Value); }
void AMaiWalkCharacter::East(float Value) { if (CanMoveInCampaign()) AddMovementInput(FVector::RightVector,Value); }
void AMaiWalkCharacter::InteractNearest() {
    if (!CanMoveInCampaign()) return; AMaiInteriorPoint* Best=nullptr; double Distance=TNumericLimits<double>::Max();
    for (TActorIterator<AMaiInteriorPoint> It(GetWorld());It;++It) {
        const double D=FVector::DistSquared(GetActorLocation(),It->GetActorLocation());
        if (D<Distance && It->CanInteract(this)) {Best=*It;Distance=D;}
    }
    if (Best) Best->Interact_Implementation(Cast<APlayerController>(GetController()));
}
