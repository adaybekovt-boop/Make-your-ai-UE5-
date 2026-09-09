#include "World/MaiGarageInterior.h"
#include "Interaction/MaiInteriorPoint.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Components/RectLightComponent.h"
AMaiGarageInterior::AMaiGarageInterior() { RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("GarageOrigin"));PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.25f; }
void AMaiGarageInterior::BeginPlay() {Super::BeginPlay();Build();}
void AMaiGarageInterior::ConfigureLocation(const FString& Id) {if(LocationId!=Id){LocationId=Id;bBuilt=false;}}
bool AMaiGarageInterior::Build() {
    if(bBuilt) return true;
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));if(!Cube || !GetWorld()) return false;
    auto* Company=GetGameInstance()?GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>():nullptr;
    const auto Definitions=Company && Company->Domain()?Company->Domain()->Definitions():mai::Catalog::Defaults();const int Index=Definitions.LocationIndex(TCHAR_TO_UTF8(*LocationId));
    if(Index<0)return false;const auto& Definition=Definitions.locations[Index];
    const int Rows=Definition.rows,Cols=Definition.cols;if(Rows<=0||Cols<=0)return false;
    HalfX=FMath::Max(330.f,(Cols*130.f+310.f)/2);HalfY=FMath::Max(300.f,((Rows-1)*225.f+360.f)/2);
    FString AssetId=LocationId==TEXT("overseas-west")?TEXT("server-hall"):LocationId==TEXT("overseas-east")?TEXT("dc-south"):LocationId;AssetId.ReplaceInline(TEXT("-"),TEXT("_"));
    const FString AssetPath=FString::Printf(TEXT("/Game/Generated/Interiors/AuthoredV2/%s/SM_%s.SM_%s"),*AssetId,*AssetId,*AssetId);
    auto* Authored=LoadObject<UStaticMesh>(nullptr,*AssetPath);if(!Authored)return false;
    // A failed spawn must not leave half a room that gets duplicated on Retry.
    for(auto* A:Spawned) if(IsValid(A)) A->Destroy();Spawned.Empty();Racks.Empty();
    TArray<UStaticMeshComponent*> Previous;GetComponents(Previous);
    for(auto* C:Previous) C->DestroyComponent();
    TArray<URectLightComponent*> OldLights;GetComponents(OldLights);for(auto* C:OldLights)C->DestroyComponent();
    auto* Furniture=NewObject<UStaticMeshComponent>(this,TEXT("AuthoredFurnishedRoom"));Furniture->SetupAttachment(RootComponent);Furniture->SetStaticMesh(Authored);Furniture->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Furniture->SetCollisionResponseToAllChannels(ECR_Block);Furniture->RegisterComponent();AddInstanceComponent(Furniture);
    auto* FloorMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Generated/Interiors/V1/M_Floor.M_Floor"));
    auto* WallMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Generated/Interiors/V1/M_Wall.M_Wall"));
    auto* TrimMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Generated/Interiors/V1/M_Trim.M_Trim"));
    const auto Block=[&](const TCHAR* Name,FVector Position,FVector Scale,UMaterialInterface* Material=nullptr,bool Collision=true) {
        auto* C=NewObject<UStaticMeshComponent>(this,Name);C->SetupAttachment(RootComponent);C->SetStaticMesh(Cube);C->SetRelativeLocation(Position);C->SetRelativeScale3D(Scale);
        C->SetMaterial(0,Material?Material:WallMaterial);
        C->SetCastShadow(Collision);
        C->SetCollisionEnabled(Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Block);C->RegisterComponent();AddInstanceComponent(C);
    };
    Block(TEXT("WalkableFloor"),FVector(0,0,-40),FVector(HalfX/50,HalfY/50,.2),FloorMaterial);
    Block(TEXT("WestWall"),FVector(-HalfX-10,0,170),FVector(.2,HalfY/50+0.4,3.4));Block(TEXT("EastWall"),FVector(HalfX+10,0,170),FVector(.2,HalfY/50+0.4,3.4));
    Block(TEXT("NorthWall"),FVector(0,HalfY+10,170),FVector(HalfX/50+0.4,.2,3.4));Block(TEXT("SouthWall"),FVector(0,-HalfY-10,170),FVector(HalfX/50+0.4,.2,3.4));
    Block(TEXT("Ceiling"),FVector(0,0,340),FVector(HalfX/50+.4,HalfY/50+.4,.2),WallMaterial);
    // Broad, shadowless fill lights keep interior lighting stable and inexpensive.
    for(int Row=0;Row<Rows;Row+=2){auto* Light=NewObject<URectLightComponent>(this);Light->SetupAttachment(RootComponent);Light->SetRelativeLocation(FVector(0,(Row-(Rows-1)*.5f)*225,305));Light->SetRelativeRotation(FRotator(-90,0,0));Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(2500);Light->SetAttenuationRadius(700);Light->SetSourceWidth(250);Light->SetSourceHeight(100);Light->SetLightColor(FLinearColor(.9,1, .97));Light->SetCastShadows(false);Light->RegisterComponent();AddInstanceComponent(Light);}
    const auto Profiles=mai::InteriorProfiles();
    for(const auto& P:Profiles.front().points) {
        if(LocationId!=TEXT("garage") && P.action=="talk")continue;
        FVector Position;
        if(P.action=="review")Position=FVector(-175,-HalfY+95,100);
        else if(P.action=="city")Position=FVector(HalfX-60,HalfY-70,70);
        else if(P.action=="talk")Position=FVector(HalfX-65,-HalfY+100,75);
        else Position=FVector(P.xCm<0?-HalfX+65:HalfX-65,HalfY-180,60);
        auto* A=GetWorld()->SpawnActor<AMaiInteriorPoint>(GetActorLocation()+Position,FRotator::ZeroRotator);
        if(!A) return false;A->LocationId=LocationId;A->Configure(UTF8_TO_TCHAR(P.id.c_str()),UTF8_TO_TCHAR(P.action.c_str()));Spawned.Add(A);
    }
    for(int32 Cell=0;Cell<Rows*Cols;++Cell) {
        auto* A=GetWorld()->SpawnActor<AMaiInteriorPoint>(GetActorLocation()+FVector((Cell%Cols-(Cols-1)*.5f)*130,(Cell/Cols-(Rows-1)*.5f)*225,5),FRotator::ZeroRotator);
        if(!A) return false;A->LocationId=LocationId;A->Configure(FString::Printf(TEXT("rack-%d"),Cell),TEXT("location"),Cell);Spawned.Add(A);Racks.Add(A);
    }
    bGraybox=false;bBuilt=true;Tick(0);return true;
}
void AMaiGarageInterior::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);auto* C=GetGameInstance()?GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>():nullptr;
    if(!C || !C->Domain()) return;const int32 Index=C->Domain()->Definitions().LocationIndex(TCHAR_TO_UTF8(*LocationId));if(Index<0) return;
    const auto& Slots=C->Domain()->View().locations[static_cast<size_t>(Index)].slots;
    for(int32 I=0;I<Racks.Num() && static_cast<size_t>(I)<Slots.size();++I) if(IsValid(Racks[I])) {
        const bool Installed=Slots[static_cast<size_t>(I)].chassis>=0;Racks[I]->SetRack(Installed);
        const auto P=Racks[I]->GetActorLocation();Racks[I]->SetActorLocation(FVector(P.X,P.Y,GetActorLocation().Z+(Installed?90:5)));
    }
}
void AMaiGarageInterior::EndPlay(const EEndPlayReason::Type Reason) {for(auto* A:Spawned) if(IsValid(A)) A->Destroy();Spawned.Empty();Racks.Empty();Super::EndPlay(Reason);}
