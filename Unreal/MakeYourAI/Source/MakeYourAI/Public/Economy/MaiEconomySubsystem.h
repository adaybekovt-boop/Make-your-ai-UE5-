#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/MaiViewTypes.h"
#include "MaiEconomySubsystem.generated.h"

// Read model and nuclear commands only. Money is posted by the one company clock.
UCLASS()
class MAKEYOURAI_API UMaiEconomySubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Economy") FMaiEconomyView GetRates() const;
    UFUNCTION(BlueprintCallable, Category="Economy") FMaiActionResult BuyNuclearStation();
};
