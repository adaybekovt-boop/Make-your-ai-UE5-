#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/MaiViewTypes.h"
#include "MaiRegionSubsystem.generated.h"

UCLASS()
class MAKEYOURAI_API UMaiRegionSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Regions") FMaiActionResult Unlock(const FString& RegionId);
    UFUNCTION(BlueprintCallable, Category="Regions") FMaiActionResult SwitchTo(const FString& RegionId);
    UFUNCTION(BlueprintPure, Category="Regions") FString GetActiveRegion() const;
    UFUNCTION(BlueprintPure, Category="Regions") int32 GetCompanyCourtRiskPpm() const;
};
