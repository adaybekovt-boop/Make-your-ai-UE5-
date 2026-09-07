#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Campaign/MaiCampaign.h"
#include "MaiCampaignAsset.generated.h"
class UTexture2D;

USTRUCT(BlueprintType)
struct FMaiDifficultyDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CapitalBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ProcurementBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DefectBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 FailureBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LegalBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HiringBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CompetitorBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TrainingBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 InsolvencyGraceHours = 48;
};
USTRUCT(BlueprintType)
struct FMaiDatasetOfferDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="2")) int32 DataType = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Volume = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int64 CostMicro = 1400000000LL;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 QualityBps = 9500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 NoiseBps = 500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LegalBps = 0;
};
USTRUCT(BlueprintType)
struct FMaiReviewCardDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Prompt;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Left;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Right;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DataType = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BetterSide = 0;
};
USTRUCT(BlueprintType)
struct FMaiEndingDefinition {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Kind = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Title;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Line;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int64 MinValueMicro = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int64 MinModelMicroIQ = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinReputation = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxLegalBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinDataQualityBps = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxDependencyBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinEmployeeCareBps = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxAutomationBps = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowReturnToSave = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RegulatorMinLegalBps = 9000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RegulatorMinIgnoredWarnings = 3;
};
UCLASS(BlueprintType)
class MAKEYOURAI_API UMaiCampaignAsset : public UPrimaryDataAsset {
    GENERATED_BODY()
public:
    UMaiCampaignAsset();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE") TArray<FMaiDifficultyDefinition> Difficulties;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE") TArray<FMaiDatasetOfferDefinition> DatasetOffers;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Review") TArray<FMaiReviewCardDefinition> ReviewCards;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE") TArray<FMaiEndingDefinition> Endings;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE", meta=(ClampMin="3", ClampMax="5")) int32 ManualSteps = 4;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE") int64 HumanHireMicro = 500000000LL;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE") int64 HumanBatchMicro = 60000000LL;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE") int64 AISetupMicro = 2000000000LL;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BALANCE_TUNABLE") int64 AIUpgradeMicro = 1000000000LL;
    // Assigned in Editor from Content/Source/Portraits/elon_max_remade.jpg. Soft ptr stays unset until a real import.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fictional") TSoftObjectPtr<UTexture2D> ElonMaxPortrait;
    bool ToDomain(mai::CampaignRules& Out, FString& Error) const;
};
