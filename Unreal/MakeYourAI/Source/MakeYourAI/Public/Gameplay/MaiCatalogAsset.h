#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gameplay/MaiViewTypes.h"
#include "MaiCatalogAsset.generated.h"

USTRUCT(BlueprintType)
struct FMaiChipDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Money") int64 PriceMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Money") int64 MaintenanceMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hardware") int32 PowerWatts = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hardware") int32 ComputeMilli = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delivery") int32 OfficialHours = 6;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delivery") int32 GreyHours = 2;
};
USTRUCT(BlueprintType)
struct FMaiChassisDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Money") int64 PriceMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hardware") int32 MaximumChipIndex = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hardware") int32 FailureMultiplierBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hardware") int32 ComputeMultiplierBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delivery") int32 OfficialHours = 6;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delivery") int32 GreyHours = 2;
};
USTRUCT(BlueprintType)
struct FMaiLocationDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString RegionId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Money") int64 PriceMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Money") int64 RentMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid") int32 Rows = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid") int32 Columns = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid") int32 PowerWatts = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance") bool bConfigured = true;
};
USTRUCT(BlueprintType)
struct FMaiRegionDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition") FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Money") int64 UnlockPriceMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region") int32 ElectricityMultiplierBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region") int32 CourtRiskMultiplierBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance") bool bConfigured = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Balance") bool bBalanceTunable = false;
};
USTRUCT(BlueprintType)
struct FMaiExtensionDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Nuclear") bool bNuclearConfigured = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Nuclear") int64 NuclearPriceMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Nuclear") int32 NuclearTariffMultiplierBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") bool bAuctionConfigured = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") FString RivalName = TEXT("Competitor");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int64 ReserveMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int64 IncrementMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int64 RivalBudgetMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int32 DurationGameMinutes = 120;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int32 RivalIntervalGameMinutes = 15;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int32 RivalAggressionPpm = 1000000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int32 ChassisIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int32 ChipIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") int32 Quantity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE|Auction") EMaiChannel Channel = EMaiChannel::Official;
};

UCLASS(BlueprintType)
class MAKEYOURAI_API UMaiCatalogAsset : public UPrimaryDataAsset {
    GENERATED_BODY()
public:
    UMaiCatalogAsset();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Catalog") TArray<FMaiChipDefinition> Chips;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Catalog") TArray<FMaiChassisDefinition> Chassis;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Catalog") TArray<FMaiLocationDefinition> Locations;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Catalog") TArray<FMaiRegionDefinition> Regions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Catalog") FMaiExtensionDefinition Extensions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Money") int64 ElectricityPerKwhMicro = 18000000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Risk") int32 GreyDefectPpm = 80000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Risk") int32 DailyFailurePpm = 2000;
    bool ToDomain(mai::Catalog& Out, FString& Error) const;
};
