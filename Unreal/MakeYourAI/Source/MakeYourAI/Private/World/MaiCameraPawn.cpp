#include "World/MaiCameraPawn.h"
#include "World/MaiPlayerController.h"
#include "UI/MaiNativeWidget.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

AMaiCameraPawn::AMaiCameraPawn() {
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("GroundAnchor"));
    Arm=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));Arm->SetupAttachment(RootComponent);
    Arm->TargetArmLength=0;Arm->bDoCollisionTest=false;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("StrategyCamera"));Camera->SetupAttachment(Arm);
    Camera->ProjectionMode=ECameraProjectionMode::Perspective;Camera->FieldOfView=65;Camera->OrthoWidth=48000;
}
void AMaiCameraPawn::SetupPlayerInputComponent(UInputComponent* Input) {
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MaiNorth"),this,&AMaiCameraPawn::MoveNorth);
    Input->BindAxis(TEXT("MaiEast"),this,&AMaiCameraPawn::MoveEast);
    Input->BindAxis(TEXT("MaiMouseX"),this,&AMaiCameraPawn::MouseHorizontal);
    Input->BindAxis(TEXT("MaiMouseY"),this,&AMaiCameraPawn::MouseVertical);
    Input->BindAxisKey(EKeys::MouseWheelAxis,this,&AMaiCameraPawn::MouseWheel);
    Input->BindAction(TEXT("MaiResetView"),IE_Pressed,this,&AMaiCameraPawn::ResetFromInput);
}
bool AMaiCameraPawn::AllowsInput(bool CheckPointer)const{
    const auto* PC=Cast<AMaiPlayerController>(GetController());
    return PC&&PC->NativeUI()&&PC->NativeUI()->CanControlMap(CheckPointer);
}
void AMaiCameraPawn::MouseWheel(float Delta){if(!FMath::IsNearlyZero(Delta)&&AllowsInput(true))Zoom(WheelZoomFactor(Delta));}
void AMaiCameraPawn::ResetFromInput(){if(AllowsInput(false))ResetOverview();}
FVector AMaiCameraPawn::GroundTarget() const {
    const FVector P=Camera->GetComponentLocation(),D=Camera->GetForwardVector();
    return P+D*(D.Z<-.05?FMath::Clamp(-P.Z/D.Z,200.,200000.):20000.);
}
void AMaiCameraPawn::RememberOverview(){Overview=GetActorTransform();bHasOverview=true;}
void AMaiCameraPawn::ResetOverview(){if(bHasOverview){SetActorTransform(Overview);Camera->SetFieldOfView(65);}}
void AMaiCameraPawn::MoveNorth(float V){
    if(!GetWorld()||FMath::IsNearlyZero(V)||!AllowsInput(false))return;
    FVector D=Camera->GetForwardVector();D.Z=0;D.Normalize();
    AddActorWorldOffset(D*V*FMath::Clamp(GetActorLocation().Z,1000.,60000.)*GetWorld()->GetDeltaSeconds());
}
void AMaiCameraPawn::MoveEast(float V){
    if(!GetWorld()||FMath::IsNearlyZero(V)||!AllowsInput(false))return;
    FVector D=Camera->GetRightVector();D.Z=0;D.Normalize();
    AddActorWorldOffset(D*V*FMath::Clamp(GetActorLocation().Z,1000.,60000.)*GetWorld()->GetDeltaSeconds());
}
void AMaiCameraPawn::Orbit(float Yaw,float Pitch){
    const FVector Pivot=GroundTarget();const double Distance=FVector::Distance(GetActorLocation(),Pivot);
    FRotator R=GetActorRotation();R.Yaw+=Yaw;R.Pitch=FMath::Clamp(R.Pitch+Pitch,-85.,-12.);R.Roll=0;
    SetActorLocationAndRotation(Pivot-R.Vector()*Distance,R);
}
void AMaiCameraPawn::MouseHorizontal(float V){
    auto* PC=Cast<APlayerController>(GetController());if(!PC||FMath::IsNearlyZero(V)||!AllowsInput(true))return;
    if(PC->IsInputKeyDown(EKeys::MiddleMouseButton)||(PC->IsInputKeyDown(EKeys::RightMouseButton)&&PC->IsInputKeyDown(EKeys::LeftShift)))Orbit(V*2,0);
    else if(PC->IsInputKeyDown(EKeys::RightMouseButton))AddActorWorldOffset(-Camera->GetRightVector()*V*FMath::Max(100.,GetActorLocation().Z)*.012);
}
void AMaiCameraPawn::MouseVertical(float V){
    auto* PC=Cast<APlayerController>(GetController());if(!PC||FMath::IsNearlyZero(V)||!AllowsInput(true))return;
    if(PC->IsInputKeyDown(EKeys::MiddleMouseButton)||(PC->IsInputKeyDown(EKeys::RightMouseButton)&&PC->IsInputKeyDown(EKeys::LeftShift)))Orbit(0,V*2);
    else if(PC->IsInputKeyDown(EKeys::RightMouseButton)){FVector D=Camera->GetForwardVector();D.Z=0;D.Normalize();AddActorWorldOffset(-D*V*FMath::Max(100.,GetActorLocation().Z)*.012);}
}
void AMaiCameraPawn::Zoom(float Factor){
    FVector Target=GroundTarget();auto* PC=Cast<APlayerController>(GetController());FVector P,D;
    if(PC&&PC->DeprojectMousePositionToWorld(P,D)&&D.Z<-.05){const double T=-P.Z/D.Z;if(T>0&&T<200000)Target=P+D*T;}
    const FVector Next=Target+(GetActorLocation()-Target)*Factor;
    if(Next.Z>=250&&Next.Z<=150000)SetActorLocation(Next);
}
void AMaiCameraPawn::ZoomIn(){Zoom(.85f);}
void AMaiCameraPawn::ZoomOut(){Zoom(1.f/.85f);}
void AMaiCameraPawn::Focus(const FVector& P,float Width){const double D=FMath::Clamp(double(Width),1200.,90000.);SetActorLocation(P-GetActorForwardVector()*D);}
