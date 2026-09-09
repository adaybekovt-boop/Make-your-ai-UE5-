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
    GetCharacterMovement()->bOrientRotationToMovement=false; bUseControllerRotationYaw=true;
    GrayboxBody=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExplicitGrayboxAvatar")); GrayboxBody->SetupAttachment(GetRootComponent());
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Shape.Succeeded()) GrayboxBody->SetStaticMesh(Shape.Object);
    GrayboxBody->SetRelativeScale3D(FVector(.55,.55,1.1)); GrayboxBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GrayboxBody->SetVisibility(false);
    Arm=CreateDefaultSubobject<USpringArmComponent>(TEXT("InteriorCameraArm")); Arm->SetupAttachment(GetRootComponent()); Arm->TargetArmLength=650.f;
    Arm->TargetArmLength=0;Arm->SetRelativeLocation(FVector(0,0,65));Arm->bUsePawnControlRotation=true;Arm->bDoCollisionTest=false;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("InteriorCamera")); Camera->SetupAttachment(Arm);
    Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;
    Camera->PostProcessSettings.AutoExposureMinBrightness=Camera->PostProcessSettings.AutoExposureMaxBrightness=5;
    Camera->PostProcessSettings.bOverride_BloomIntensity=true;Camera->PostProcessSettings.BloomIntensity=.05f;
}
bool AMaiWalkCharacter::CanMoveInCampaign() const {
    auto* C=GetGameInstance()?GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>():nullptr;
    return C && C->CampaignDomain() && C->CampaignDomain()->CanPlay() && C->CampaignDomain()->View().screen==mai::Screen::Gameplay && !C->CampaignDomain()->View().interior.empty();
}
void AMaiWalkCharacter::SetRoomBounds(const FVector& Origin,const FVector2D& HalfSize) {
    RoomOrigin=Origin;RoomHalfSize=HalfSize;SafeSpawn=GetActorLocation();bRoomBoundsSet=true;
}
void AMaiWalkCharacter::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);
    if(!bRoomBoundsSet)return;
    const FVector Local=GetActorLocation()-RoomOrigin;
    // Solid walls are the primary boundary. Recover if physics or a bad spawn escapes.
    if(Local.ContainsNaN() || FMath::Abs(Local.X)>RoomHalfSize.X || FMath::Abs(Local.Y)>RoomHalfSize.Y || Local.Z < -40 || Local.Z>600) {
        GetCharacterMovement()->StopMovementImmediately();
        SetActorLocation(SafeSpawn,false,nullptr,ETeleportType::TeleportPhysics);
    }
}
void AMaiWalkCharacter::SetupPlayerInputComponent(UInputComponent* Input) {
    Super::SetupPlayerInputComponent(Input); Input->BindAxis(TEXT("MaiNorth"),this,&AMaiWalkCharacter::North); Input->BindAxis(TEXT("MaiEast"),this,&AMaiWalkCharacter::East);
    Input->BindAction(TEXT("MaiInteract"),IE_Pressed,this,&AMaiWalkCharacter::InteractNearest);
    Input->BindAxis(TEXT("MaiMouseX"),this,&AMaiWalkCharacter::LookX);Input->BindAxis(TEXT("MaiMouseY"),this,&AMaiWalkCharacter::LookY);
}
void AMaiWalkCharacter::North(float Value) { if (CanMoveInCampaign()) AddMovementInput(GetActorForwardVector(),Value); }
void AMaiWalkCharacter::East(float Value) { if (CanMoveInCampaign()) AddMovementInput(GetActorRightVector(),Value); }
void AMaiWalkCharacter::LookX(float Value){auto* PC=Cast<APlayerController>(GetController());if(CanMoveInCampaign()&&PC&&PC->IsInputKeyDown(EKeys::RightMouseButton))AddControllerYawInput(Value);}
void AMaiWalkCharacter::LookY(float Value){auto* PC=Cast<APlayerController>(GetController());if(CanMoveInCampaign()&&PC&&PC->IsInputKeyDown(EKeys::RightMouseButton))AddControllerPitchInput(-Value);}
void AMaiWalkCharacter::InteractNearest() {
    if (!CanMoveInCampaign()) return; AMaiInteriorPoint* Best=nullptr; double Distance=TNumericLimits<double>::Max();
    for (TActorIterator<AMaiInteriorPoint> It(GetWorld());It;++It) {
        const double D=FVector::DistSquared(GetActorLocation(),It->GetActorLocation());
        if (D<Distance && It->CanInteract(this)) {Best=*It;Distance=D;}
    }
    if (Best) Best->Interact_Implementation(Cast<APlayerController>(GetController()));
}
