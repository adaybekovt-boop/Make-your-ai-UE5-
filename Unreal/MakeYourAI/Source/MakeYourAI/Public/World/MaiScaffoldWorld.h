#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MaiScaffoldWorld.generated.h"
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class AMaiLocationActor;

UCLASS()
class MAKEYOURAI_API AMaiScaffoldWorld : public AActor {
    GENERATED_BODY()
public:
    AMaiScaffoldWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    bool Enter(const FString& LocationId);
    FVector FindLandmark(const FString& Id) const;
    FVector InteriorOrigin() const { return FVector(100000, 100000, 0); }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scene") bool bImportedCity = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scene") TMap<FString, FVector> MarkerPositions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scene") TSoftObjectPtr<UStaticMesh> GarageInteriorMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scene") TSoftObjectPtr<UStaticMesh> GarageEnclosureMesh;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RepeatedProps;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> InstalledRacks;
    // UPROPERTY keeps these actor arrays visible to GC. Raw element pointers also
    // match the explicit pointer iteration used by the scoped interior cleanup.
    UPROPERTY(Transient) TArray<AMaiLocationActor*> Cells;
    UPROPERTY(Transient) TArray<AActor*> InteriorActors;
    FString InteriorId;
    FString LastRackSignature;
    void BuildGraybox();
    void BuildMarkers();
    void RefreshRacks();
};
