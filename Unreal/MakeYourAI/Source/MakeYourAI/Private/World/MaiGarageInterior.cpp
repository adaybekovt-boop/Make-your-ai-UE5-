#include "World/MaiGarageInterior.h"
#include "Interaction/MaiInteriorPoint.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
AMaiGarageInterior::AMaiGarageInterior() { RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("GarageOrigin"));PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.25f; }
void AMaiGarageInterior::BeginPlay() {Super::BeginPlay();Build();}
bool AMaiGarageInterior::Build() {
    if(bBuilt) return true;
    auto* Cube=FindObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));if(!Cube || !GetWorld()) return false;
    // A failed spawn must not leave half a room that gets duplicated on Retry.
    for(auto* A:Spawned) if(IsValid(A)) A->Destroy();Spawned.Empty();Racks.Empty();
    TArray<UStaticMeshComponent*> Previous;GetComponents(Previous);
    for(auto* C:Previous) C->DestroyComponent();
    const auto Block=[&](const TCHAR* Name,FVector Position,FVector Scale) {
        auto* C=NewObject<UStaticMeshComponent>(this,Name);C->SetupAttachment(RootComponent);C->SetStaticMesh(Cube);C->SetRelativeLocation(Position);C->SetRelativeScale3D(Scale);
        C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);C->SetCollisionResponseToAllChannels(ECR_Block);C->RegisterComponent();AddInstanceComponent(C);
    };
    Block(TEXT("WalkableFloor"),FVector(0,0,-10),FVector(14,14,.2));
    Block(TEXT("WestWall"),FVector(-710,0,150),FVector(.2,14,3));Block(TEXT("EastWall"),FVector(710,0,150),FVector(.2,14,3));
    Block(TEXT("NorthWall"),FVector(0,710,150),FVector(14,.2,3));Block(TEXT("SouthWall"),FVector(0,-710,150),FVector(14,.2,3));
    for(const auto& P:mai::InteriorProfiles().front().points) {
        auto* A=GetWorld()->SpawnActor<AMaiInteriorPoint>(GetActorLocation()+FVector(P.xCm,P.yCm,50),FRotator::ZeroRotator);
        if(!A) return false;A->Configure(UTF8_TO_TCHAR(P.id.c_str()),UTF8_TO_TCHAR(P.action.c_str()));Spawned.Add(A);
    }
    for(int32 Cell=0;Cell<9;++Cell) {
        auto* A=GetWorld()->SpawnActor<AMaiInteriorPoint>(GetActorLocation()+FVector((Cell%3-1)*150,Cell/3*180,5),FRotator::ZeroRotator);
        if(!A) return false;A->Configure(FString::Printf(TEXT("rack-%d"),Cell),TEXT("location"),Cell);Spawned.Add(A);Racks.Add(A);
    }
    bBuilt=true;Tick(0);return true;
}
void AMaiGarageInterior::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);auto* C=GetGameInstance()?GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>():nullptr;
    if(!C || !C->Domain()) return;const int32 Index=C->Domain()->Definitions().LocationIndex("garage");if(Index<0) return;
    const auto& Slots=C->Domain()->View().locations[static_cast<size_t>(Index)].slots;
    for(int32 I=0;I<Racks.Num() && static_cast<size_t>(I)<Slots.size();++I) if(IsValid(Racks[I])) {
        const bool Installed=Slots[static_cast<size_t>(I)].chassis>=0;Racks[I]->SetRack(Installed);
        const auto P=Racks[I]->GetActorLocation();Racks[I]->SetActorLocation(FVector(P.X,P.Y,GetActorLocation().Z+(Installed?90:5)));
    }
}
void AMaiGarageInterior::EndPlay(const EEndPlayReason::Type Reason) {for(auto* A:Spawned) if(IsValid(A)) A->Destroy();Spawned.Empty();Racks.Empty();Super::EndPlay(Reason);}
