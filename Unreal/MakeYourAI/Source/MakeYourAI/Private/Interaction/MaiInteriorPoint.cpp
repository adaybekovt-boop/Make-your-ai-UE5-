#include "Interaction/MaiInteriorPoint.h"
#include "World/MaiWalkCharacter.h"
#include "World/MaiPlayerController.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
AMaiInteriorPoint::AMaiInteriorPoint() {
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteractionCollision"));RootComponent=Body;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));if(Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(.65,.65,.9));Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Body->SetCollisionResponseToAllChannels(ECR_Block);
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("ActionLabel"));Label->SetupAttachment(Body);Label->SetRelativeLocation(FVector(0,0,90));Label->SetWorldSize(25.f);
}
void AMaiInteriorPoint::Configure(const FString& Id,const FString& Command,int32 InCell) {
    PointId=Id;Action=Command;Cell=InCell;Label->SetText(FText::FromString(Id+TEXT(" [E]")));
    if (Action==TEXT("talk") && !Proximity) {
        Tags.AddUnique(TEXT("CampaignGarageNpc"));Proximity=NewObject<UMaiProximityComponent>(this,TEXT("AdvisorProximity"));Proximity->RegisterComponent();
        Proximity->OnStateChanged.AddDynamic(this,&AMaiInteriorPoint::NpcState);
    }
}
void AMaiInteriorPoint::NpcState(EMaiNpcState State,FText Line) {Label->SetText(State==EMaiNpcState::Idle?FText::FromString(TEXT("Advisor [E]")):Line);}
void AMaiInteriorPoint::SetRack(bool bInstalled) {Body->SetRelativeScale3D(FVector(.6,.8,bInstalled?1.8:.1));Label->SetText(FText::FromString(FString::Printf(TEXT("Cell %d / %s [E]"),Cell+1,bInstalled?TEXT("rack"):TEXT("empty"))));}
bool AMaiInteriorPoint::CanInteract(const AMaiWalkCharacter* Character) const {
    if (!Character || !GetWorld() || IsHidden() || !FMath::IsFinite(RangeCm) || RangeCm<=0 || RangeCm>1000) return false;
    if(FVector::DistSquared(GetActorLocation(),Character->GetActorLocation())>FMath::Square(RangeCm)) return false;
    FVector Eye;FRotator Rot;Character->GetActorEyesViewPoint(Eye,Rot);FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(MaiInteriorInteract),false,Character);
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,Eye,GetActorLocation(),ECC_Visibility,Params);
    return !Blocked || Hit.GetActor()==this;
}
void AMaiInteriorPoint::Interact_Implementation(APlayerController* Player) {
    auto* PC=Cast<AMaiPlayerController>(Player);auto* Character=PC?Cast<AMaiWalkCharacter>(PC->GetPawn()):nullptr;
    auto* C=GetGameInstance()?GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>():nullptr;
    if(!C || !C->CampaignDomain() || !C->CampaignDomain()->CanPlay() || C->CampaignDomain()->View().interior!=TCHAR_TO_UTF8(*LocationId) || !CanInteract(Character)) return;
    if(Action==TEXT("city")) PC->ShowCity();
    else if(Action==TEXT("review")) PC->OpenTraining(true);
    else if(Action==TEXT("talk")) C->CampaignTransact([](mai::Campaign& G){return G.TalkToGarageNpc();});
    else if(Action==TEXT("warehouse")) PC->ShowWarehouse();
    else PC->SelectLocation(LocationId,Cell);
}
