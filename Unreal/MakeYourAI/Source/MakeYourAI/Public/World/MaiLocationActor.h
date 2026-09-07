#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MaiInteractable.h"
#include "MaiLocationActor.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

UCLASS()
class MAKEYOURAI_API AMaiLocationActor : public AActor, public IMaiInteractable {
    GENERATED_BODY()
public:
    AMaiLocationActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void Interact_Implementation(APlayerController* Player) override;
    void Configure(const FString& Id, const FString& Label, int32 InCell = -1, FVector Size = FVector(8, 8, 5));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Location") FString LocationId = TEXT("garage");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Location") int32 CellIndex = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Location") FString DisplayLabel = TEXT("Garage");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UBoxComponent> InteractionBox;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UTextRenderComponent> Label;
private:
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Tint;
    int32 LastStatus = -1;
};
