#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MaiInteractable.generated.h"
class APlayerController;

UINTERFACE(BlueprintType)
class MAKEYOURAI_API UMaiInteractable : public UInterface { GENERATED_BODY() };
class MAKEYOURAI_API IMaiInteractable {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction") void Interact(APlayerController* Player);
};
