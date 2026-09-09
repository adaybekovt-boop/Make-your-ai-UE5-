#include "Interaction/MaiInteriorPoint.h"
#include "World/MaiWalkCharacter.h"
#include "World/MaiPlayerController.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
AMaiInteriorPoint::AMaiInteriorPoint() {
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteractionCollision"));RootComponent=Body;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));if(Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(.65,.65,.9));Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Body->SetCollisionResponseToAllChannels(ECR_Block);
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("ActionLabel"));Label->SetupAttachment(Body);Label->SetRelativeLocation(FVector(0,0,90));Label->SetWorldSize(16.f);
    Label->SetAbsolute(false,true,true);Label->SetWorldRotation(FRotator(0,135,0));Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetTextRenderColor(FColor(170,218,206));
}
void AMaiInteriorPoint::Configure(const FString& Id,const FString& Command,int32 InCell) {
    PointId=Id;Action=Command;Cell=InCell;Label->SetText(FText::FromString(Id+TEXT(" [E]")));
    Label->SetVisibility(false); // Localized hints use UMG; the engine 3D font lacks Cyrillic.
    if(Cell<0){Body->SetVisibility(false);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
    Body->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Cell>=0?TEXT("/Game/Generated/Interiors/V1/M_Rack.M_Rack"):TEXT("/Game/Generated/Interiors/V1/M_Accent.M_Accent")));
    if(Cell>=0 && RackPanels.IsEmpty()) {
        for(int I=0;I<6;++I){auto* Panel=NewObject<UStaticMeshComponent>(this);Panel->SetupAttachment(Body);Panel->SetStaticMesh(Body->GetStaticMesh());Panel->SetRelativeLocation(FVector(0,-51,-38+I*14));Panel->SetRelativeScale3D(FVector(.88,.025,.1));Panel->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Generated/Interiors/V1/M_Metal.M_Metal")));Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);Panel->SetCastShadow(false);Panel->RegisterComponent();AddInstanceComponent(Panel);RackPanels.Add(Panel);}
    }
    if(Cell<0){const TMap<FString,FString> Names={{TEXT("location"),TEXT("Управление")},{TEXT("warehouse"),TEXT("Склад")},{TEXT("review"),TEXT("Проверка данных")},{TEXT("talk"),TEXT("Советник")},{TEXT("city"),TEXT("Выход")}};if(const auto* Name=Names.Find(Action))Label->SetText(FText::FromString(*Name+TEXT(" [E]")));}
    if (Action==TEXT("talk") && !Proximity) {
        Tags.AddUnique(TEXT("CampaignGarageNpc"));Proximity=NewObject<UMaiProximityComponent>(this,TEXT("AdvisorProximity"));Proximity->RegisterComponent();
        Proximity->OnStateChanged.AddDynamic(this,&AMaiInteriorPoint::NpcState);
    }
}
void AMaiInteriorPoint::NpcState(EMaiNpcState State,FText Line) {Label->SetText(State==EMaiNpcState::Idle?FText::FromString(TEXT("Advisor [E]")):Line);}
FString AMaiInteriorPoint::InteractionLabel() const {
    if(Cell>=0)return FString::Printf(TEXT("Ячейка %d — оборудование"),Cell+1);
    const TMap<FString,FString> Names={{TEXT("location"),TEXT("Управление площадкой")},{TEXT("warehouse"),TEXT("Закупки и склад")},{TEXT("review"),TEXT("Проверка данных")},{TEXT("talk"),TEXT("Поговорить с советником")},{TEXT("city"),TEXT("Выйти в город")}};
    const auto* Name=Names.Find(Action);return Name?*Name:PointId;
}
void AMaiInteriorPoint::SetRack(bool bInstalled) {
    if(LastRackState==int32(bInstalled))return;LastRackState=int32(bInstalled);
    Body->SetRelativeScale3D(FVector(.6,.8,bInstalled?1.8:.1));
    Body->SetVisibility(bInstalled);Body->SetCollisionEnabled(bInstalled?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::QueryOnly);
    for(const auto& Panel:RackPanels)if(Panel)Panel->SetVisibility(bInstalled);
    Label->SetRelativeLocation(FVector(0,0,bInstalled?65:220));
    Label->SetText(FText::FromString(FString::Printf(TEXT("%d · %s [E]"),Cell+1,bInstalled?TEXT("Сервер"):TEXT("Свободно"))));
}
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
