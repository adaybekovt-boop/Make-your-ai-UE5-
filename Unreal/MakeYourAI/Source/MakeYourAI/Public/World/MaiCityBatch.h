#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MaiInteractable.h"
#include "MaiCityBatch.generated.h"
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
UCLASS()
class MAKEYOURAI_API AMaiCityBatch : public AActor, public IMaiInteractable {
    GENERATED_BODY()
public:
    AMaiCityBatch();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Instances;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString LocationId;
    UFUNCTION(BlueprintCallable, Category="MakeYourAI|City")
    bool Configure(UStaticMesh* Mesh, const TArray<UMaterialInterface*>& Materials, const FString& MatricesJson, const FString& GameLocation);
    virtual void Interact_Implementation(APlayerController* Player) override;
};
