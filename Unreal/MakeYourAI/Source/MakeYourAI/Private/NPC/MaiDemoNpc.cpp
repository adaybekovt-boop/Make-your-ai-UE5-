#include "NPC/MaiDemoNpc.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMaiDemoNpc::AMaiDemoNpc() {
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrayboxPerson")); RootComponent = Body;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Mesh.Succeeded()) Body->SetStaticMesh(Mesh.Object);
    Body->SetRelativeScale3D(FVector(0.5, 0.5, 1.76)); Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NpcLine")); Label->SetupAttachment(Body);
    Label->SetRelativeLocation(FVector(0, 0, 125)); Label->SetRelativeScale3D(FVector(2, 2, 0.568));
    Label->SetWorldSize(40); Label->SetText(FText::FromString(TEXT("Garage contact [graybox NPC]")));
    Proximity = CreateDefaultSubobject<UMaiProximityComponent>(TEXT("ProximityEvent"));
}
void AMaiDemoNpc::BeginPlay() { Super::BeginPlay(); Proximity->OnStateChanged.AddDynamic(this, &AMaiDemoNpc::OnNpcState); }
void AMaiDemoNpc::OnNpcState(EMaiNpcState State, FText Line) {
    Label->SetText(State == EMaiNpcState::Idle ? FText::FromString(TEXT("Garage contact")) : Line);
    Label->SetTextRenderColor(State == EMaiNpcState::PhoneCall ? FColor(235, 193, 90) : FColor::White);
}
