#include "World/MaiScaffoldWorld.h"
#include "World/MaiLocationActor.h"
#include "NPC/MaiDemoNpc.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/SkyAtmosphere.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

namespace {
AStaticMeshActor* MeshActor(UWorld* World, UStaticMesh* Mesh, const FVector& Position, const FVector& Scale, bool bCollision) {
    auto* Actor = World->SpawnActor<AStaticMeshActor>(Position, FRotator::ZeroRotator);
    if (!Actor) return nullptr;
    auto* Component = Actor->GetStaticMeshComponent(); Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(Mesh); Component->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Actor->SetActorScale3D(Scale); return Actor;
}
}
AMaiScaffoldWorld::AMaiScaffoldWorld() {
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickInterval = 0.2f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RepeatedProps = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RepeatedGrayboxProps")); RepeatedProps->SetupAttachment(RootComponent);
    InstalledRacks = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("EquipmentProxyInstances")); InstalledRacks->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) { RepeatedProps->SetStaticMesh(Cube.Object); InstalledRacks->SetStaticMesh(Cube.Object); }
    RepeatedProps->SetCollisionEnabled(ECollisionEnabled::NoCollision); InstalledRacks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AMaiScaffoldWorld::BeginPlay() {
    Super::BeginPlay();
    if (!bImportedCity) BuildGraybox();
    BuildMarkers();
    const FVector Garage = FindLandmark(TEXT("garage"));
    GetWorld()->SpawnActor<AMaiDemoNpc>(Garage + FVector(650, 0, 0), FRotator::ZeroRotator);
}
void AMaiScaffoldWorld::BuildGraybox() {
    // Logical test layout only. It is explicitly NOT CityV4 or a replacement city.
    MarkerPositions = {
        {TEXT("garage"), FVector(-9000, -7000, 250)}, {TEXT("workshop"), FVector(-6500, -7000, 300)},
        {TEXT("technopark"), FVector(-1500, 1000, 500)}, {TEXT("server-hall"), FVector(2000, 1000, 500)},
        {TEXT("campus"), FVector(5000, 6000, 400)}, {TEXT("dc-north"), FVector(-9000, 11000, 500)},
        {TEXT("dc-south"), FVector(10000, -9000, 500)}, {TEXT("auction"), FVector(-3000, -4000, 300)},
        {TEXT("nuclear-power"), FVector(-25000, 3000, 800)}, {TEXT("greenhaven"), FVector(23000, -8000, 150)},
        {TEXT("hq"), FVector(1500, 5500, 400)}};
    auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    MeshActor(GetWorld(), Cube, FVector(0, 0, -60), FVector(600, 360, 1), true);
    for (int32 I = 0; I < 16; ++I) RepeatedProps->AddInstance(FTransform(FRotator::ZeroRotator, FVector(-9000 + I * 700, -4500, 150), FVector(0.25, 0.25, 3)));
    auto* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 30000), FRotator(-40, -35, 0));
    if (Sun) { Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable); Sun->GetLightComponent()->SetIntensity(3); }
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    auto* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky) { Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable); Sky->GetLightComponent()->SetIntensity(0.8f); Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true); }
}
void AMaiScaffoldWorld::BuildMarkers() {
    const auto* C = GetWorld()->GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    for (const auto& Pair : MarkerPositions) {
        FString Name = Pair.Key;
        if (C && C->Domain()) {
            const int I = C->Domain()->Definitions().LocationIndex(TCHAR_TO_UTF8(*Pair.Key));
            if (I >= 0) Name = UTF8_TO_TCHAR(C->Domain()->Definitions().locations[static_cast<std::size_t>(I)].name.c_str());
        }
        if (Pair.Key == TEXT("nuclear-power")) Name = TEXT("Nuclear Power Station");
        if (Pair.Key == TEXT("greenhaven")) Name = TEXT("Greenhaven Suburb");
        auto* Marker = GetWorld()->SpawnActor<AMaiLocationActor>(Pair.Value, FRotator::ZeroRotator);
        if (Marker) {
            Marker->Configure(Pair.Key, Name, -1, bImportedCity ? FVector(2.5, 2.5, 0.4) : FVector(8, 8, FMath::Max(1.f, static_cast<float>(Pair.Value.Z) / 50.f)));
            if (bImportedCity) Marker->Body->SetCastShadow(false);
        }
    }
    if (bImportedCity && MarkerPositions.Num() == 0) UE_LOG(LogTemp, Error, TEXT("Imported scene has no marker bounds; no graybox placement will be substituted."));
}
FVector AMaiScaffoldWorld::FindLandmark(const FString& Id) const { const auto* Value = MarkerPositions.Find(Id); return Value ? *Value : FVector::ZeroVector; }
bool AMaiScaffoldWorld::Enter(const FString& Id) {
    auto* C = GetWorld()->GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if (!C || !C->Domain() || C->GetLocationStatus(Id) != EMaiLocationStatus::Owned) return false;
    const int Index = C->Domain()->Definitions().LocationIndex(TCHAR_TO_UTF8(*Id));
    if (Index < 0) return false;
    for (auto* Cell : Cells) if (IsValid(Cell)) Cell->Destroy(); Cells.Reset();
    for (auto* Actor : InteriorActors) if (IsValid(Actor)) Actor->Destroy(); InteriorActors.Reset();
    InstalledRacks->ClearInstances(); LastRackSignature.Empty(); InteriorId = Id;
    const auto& D = C->Domain()->Definitions().locations[static_cast<std::size_t>(Index)];
    auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    InteriorActors.Add(MeshActor(GetWorld(), Cube, InteriorOrigin() - FVector(0, 0, 15), FVector((D.cols * 125 + 300) / 100.f, (D.rows * 280 + 300) / 100.f, 0.3), true));
    if (Id == TEXT("garage") && !GarageInteriorMesh.IsNull()) {
        if (auto* Mesh = GarageInteriorMesh.LoadSynchronous()) {
            const auto Bounds = Mesh->GetBounds();
            const FVector Offset = InteriorOrigin() - FVector(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z - Bounds.BoxExtent.Z);
            InteriorActors.Add(MeshActor(GetWorld(), Mesh, Offset, FVector::OneVector, true));
            if (auto* Enclosure = GarageEnclosureMesh.LoadSynchronous()) {
                auto* Actor = MeshActor(GetWorld(), Enclosure, Offset, FVector::OneVector, true);
                // Separate imported UCX/shell collision. No bounding box filling the whole room.
                if (Actor) { Actor->GetStaticMeshComponent()->SetVisibility(false); InteriorActors.Add(Actor); }
            }
        }
    }
    for (int32 Row = 0; Row < D.rows; ++Row) for (int32 Col = 0; Col < D.cols; ++Col) {
        const FVector Position = InteriorOrigin() + FVector((Col - (D.cols - 1) / 2.f) * 125.f, (Row - (D.rows - 1) / 2.f) * 280.f, 8);
        auto* Cell = GetWorld()->SpawnActor<AMaiLocationActor>(Position, FRotator::ZeroRotator);
        if (Cell) { Cell->Configure(Id, FString::Printf(TEXT("%d:%d"), Row + 1, Col + 1), Row * D.cols + Col, FVector(0.9, 1.1, 0.12)); Cells.Add(Cell); }
    }
    RefreshRacks(); return true;
}
void AMaiScaffoldWorld::RefreshRacks() {
    auto* C = GetWorld()->GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if (!C || !C->Domain() || InteriorId.IsEmpty()) return;
    const auto& Sim = *C->Domain(); const int Index = Sim.Definitions().LocationIndex(TCHAR_TO_UTF8(*InteriorId));
    if (Index < 0) return;
    const auto& L = Sim.View().locations[static_cast<std::size_t>(Index)]; const auto& D = Sim.Definitions().locations[static_cast<std::size_t>(Index)];
    FString Signature = FString::Printf(TEXT("%llu:%d:"), static_cast<unsigned long long>(C->Generation()), L.owned ? 1 : 0);
    for (const auto& Slot : L.slots) Signature += FString::Printf(TEXT("%d,%d;"), Slot.chassis, Slot.chip);
    if (Signature == LastRackSignature) return; LastRackSignature = Signature; InstalledRacks->ClearInstances();
    if (!L.owned) return;
    for (std::size_t I = 0; I < L.slots.size(); ++I) {
        const auto& Slot = L.slots[I]; if (Slot.chassis < 0) continue;
        const float Height = Slot.chip < 0 ? 140.f : 180.f + Slot.chassis * 10.f;
        const FVector P = InteriorOrigin() + FVector((static_cast<int32>(I) % D.cols - (D.cols - 1) / 2.f) * 125.f, (static_cast<int32>(I) / D.cols - (D.rows - 1) / 2.f) * 280.f, Height / 2 + 15);
        InstalledRacks->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(0.6, 0.9, Height / 100.f)));
    }
}
void AMaiScaffoldWorld::Tick(float DeltaSeconds) { Super::Tick(DeltaSeconds); RefreshRacks(); }
