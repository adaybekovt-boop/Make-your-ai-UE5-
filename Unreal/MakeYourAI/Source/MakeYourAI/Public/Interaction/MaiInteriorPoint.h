#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MaiInteractable.h"
#include "NPC/MaiProximityComponent.h"
#include "MaiInteriorPoint.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;
class AMaiWalkCharacter;
UCLASS()
class MAKEYOURAI_API AMaiInteriorPoint : public AActor, public IMaiInteractable {
    GENERATED_BODY()
public:
    AMaiInteriorPoint();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") FString PointId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") FString LocationId=TEXT("garage");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") FString Action;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") int32 Cell=-1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") float RangeCm=220.f;
    void Configure(const FString& Id,const FString& Command,int32 InCell=-1);
    bool CanInteract(const AMaiWalkCharacter* Character) const;
    virtual void Interact_Implementation(APlayerController* Player) override;
    void SetRack(bool bInstalled);
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(Transient) TObjectPtr<UMaiProximityComponent> Proximity;
    UFUNCTION() void NpcState(EMaiNpcState State,FText Line);
};
