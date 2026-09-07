#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPC/MaiProximityComponent.h"
#include "MaiDemoNpc.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class MAKEYOURAI_API AMaiDemoNpc : public AActor {
    GENERATED_BODY()
public:
    AMaiDemoNpc();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC") TObjectPtr<UMaiProximityComponent> Proximity;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC") TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC") TObjectPtr<UTextRenderComponent> Label;
private:
    UFUNCTION() void OnNpcState(EMaiNpcState State, FText Line);
};
