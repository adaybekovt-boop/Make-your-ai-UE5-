#pragma once
#include "CoreMinimal.h"
#include "Core/MaiDomain.h"
#include "MaiViewTypes.generated.h"

UENUM(BlueprintType)
enum class EMaiChannel : uint8 { Official, Grey };
UENUM(BlueprintType)
enum class EMaiItemKind : uint8 { Chassis, Chip };
UENUM(BlueprintType)
enum class EMaiLocationStatus : uint8 { Locked, Available, Owned };

USTRUCT(BlueprintType)
struct MAKEYOURAI_API FMaiActionResult {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="MakeYourAI") bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly, Category="MakeYourAI") FText Message;
    static FMaiActionResult From(const mai::Result& Result) {
        FMaiActionResult Out; Out.bSuccess = Result.ok;
        Out.Message = FText::FromString(UTF8_TO_TCHAR(Result.message.c_str())); return Out;
    }
};
USTRUCT(BlueprintType)
struct MAKEYOURAI_API FMaiEconomyView {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Economy") int64 RevenuePerHourMicro = 0;
    UPROPERTY(BlueprintReadOnly, Category="Economy") int64 ElectricityPerHourMicro = 0;
    UPROPERTY(BlueprintReadOnly, Category="Economy") int64 MaintenancePerHourMicro = 0;
    UPROPERTY(BlueprintReadOnly, Category="Economy") int64 RentPerHourMicro = 0;
    UPROPERTY(BlueprintReadOnly, Category="Economy") int64 ProfitPerHourMicro = 0;
    UPROPERTY(BlueprintReadOnly, Category="Economy") int64 ComputeMilli = 0;
    static FMaiEconomyView From(const mai::Rates& Rates) {
        FMaiEconomyView Out; Out.RevenuePerHourMicro = Rates.revenue;
        Out.ElectricityPerHourMicro = Rates.electricity; Out.MaintenancePerHourMicro = Rates.maintenance;
        Out.RentPerHourMicro = Rates.rent; Out.ProfitPerHourMicro = Rates.revenue - Rates.Expenses();
        Out.ComputeMilli = Rates.computeMilli; return Out;
    }
};
USTRUCT(BlueprintType)
struct MAKEYOURAI_API FMaiOrderView {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Orders") int64 Id = 0;
    UPROPERTY(BlueprintReadOnly, Category="Orders") FString LocationId;
    UPROPERTY(BlueprintReadOnly, Category="Orders") FString ItemId;
    UPROPERTY(BlueprintReadOnly, Category="Orders") int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly, Category="Orders") EMaiChannel Channel = EMaiChannel::Official;
    UPROPERTY(BlueprintReadOnly, Category="Orders") int64 PaidMicro = 0;
    UPROPERTY(BlueprintReadOnly, Category="Orders") double RemainingGameHours = 0;
};
