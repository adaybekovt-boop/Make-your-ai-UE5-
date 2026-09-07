#include "World/MaiLocationActor.h"
#include "World/MaiPlayerController.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AMaiLocationActor::AMaiLocationActor() {
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickInterval = 0.2f;
    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionProxy")); RootComponent = InteractionBox;
    InteractionBox->SetBoxExtent(FVector(400, 400, 250));
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore); InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrayboxBody")); Body->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Mesh.Succeeded()) Body->SetStaticMesh(Mesh.Object);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision); Body->SetRelativeScale3D(FVector(8, 8, 5));
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LocationLabel")); Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, 0, 350)); Label->SetRelativeRotation(FRotator(0, 225, 0));
    Label->SetHorizontalAlignment(EHTA_Center); Label->SetWorldSize(100);
}
void AMaiLocationActor::Configure(const FString& Id, const FString& Text, int32 InCell, FVector Size) {
    LocationId = Id; DisplayLabel = Text; CellIndex = InCell;
    InteractionBox->SetBoxExtent(Size * 50); Body->SetRelativeScale3D(Size);
    Label->SetRelativeLocation(FVector(0, 0, Size.Z * 50 + (CellIndex >= 0 ? 35 : 120)));
    Label->SetWorldSize(CellIndex >= 0 ? 24 : 100); Label->SetText(FText::FromString(DisplayLabel)); LastStatus = -1;
}
void AMaiLocationActor::BeginPlay() {
    Super::BeginPlay(); Label->SetText(FText::FromString(DisplayLabel));
    if (auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Scaffold/Materials/M_ScaffoldBase.M_ScaffoldBase"))) {
        Tint = UMaterialInstanceDynamic::Create(Material, this); Body->SetMaterial(0, Tint);
    }
}
void AMaiLocationActor::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);
    auto* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const auto* C = GI ? GI->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (!C || !C->Domain()) return;
    const int32 Status = static_cast<int32>(C->GetLocationStatus(LocationId));
    const FColor Color = Status == 2 ? FColor(134, 199, 169) : Status == 1 ? FColor(211, 211, 211) : FColor(209, 165, 88);
    Label->SetTextRenderColor(Color);
    if (Status != LastStatus && CellIndex < 0) {
        LastStatus = Status;
        const bool bExtension = LocationId == TEXT("auction") || LocationId == TEXT("nuclear-power") || LocationId == TEXT("greenhaven");
        Label->SetText(FText::FromString(DisplayLabel + TEXT("\n") + (bExtension ? TEXT("Extension / BALANCE_TUNABLE") : Status == 2 ? TEXT("Owned") : Status == 1 ? TEXT("Available") : TEXT("Locked"))));
        if (Tint) Tint->SetVectorParameterValue(TEXT("BaseTint"), FLinearColor(Color) * 0.35f);
    }
    if (CellIndex >= 0) {
        const auto& Sim = *C->Domain(); const int I = Sim.Definitions().LocationIndex(TCHAR_TO_UTF8(*LocationId));
        if (I >= 0 && CellIndex < static_cast<int32>(Sim.View().locations[static_cast<std::size_t>(I)].slots.size())) {
            const auto& Slot = Sim.View().locations[static_cast<std::size_t>(I)].slots[static_cast<std::size_t>(CellIndex)];
            FString State = Slot.chassis < 0 ? TEXT("Empty") : Slot.chip < 0 ? TEXT("Rack") : UTF8_TO_TCHAR(Sim.Definitions().chips[static_cast<std::size_t>(Slot.chip)].name.c_str());
            Label->SetText(FText::FromString(DisplayLabel + TEXT("\n") + State));
        }
    }
}
void AMaiLocationActor::Interact_Implementation(APlayerController* Player) {
    if (auto* PC = Cast<AMaiPlayerController>(Player)) PC->SelectLocation(LocationId, CellIndex);
}
