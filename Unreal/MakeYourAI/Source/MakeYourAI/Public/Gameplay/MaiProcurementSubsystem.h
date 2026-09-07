#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/MaiViewTypes.h"
#include "MaiProcurementSubsystem.generated.h"

UCLASS()
class MAKEYOURAI_API UMaiProcurementSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Procurement") FMaiActionResult OrderKit(const FString& LocationId, int32 Rack, int32 Chip, EMaiChannel Channel, int32 Quantity, int32 Cell);
    UFUNCTION(BlueprintCallable, Category="Procurement") FMaiActionResult OrderItem(const FString& LocationId, EMaiItemKind Kind, int32 Item, EMaiChannel Channel, int32 Quantity, int32 Cell);
    UFUNCTION(BlueprintCallable, Category="Procurement") FMaiActionResult MountChassis(const FString& LocationId, int32 Cell, int32 Rack);
    UFUNCTION(BlueprintCallable, Category="Procurement") FMaiActionResult MountChip(const FString& LocationId, int32 Cell, int32 Chip);
    UFUNCTION(BlueprintCallable, Category="Procurement") FMaiActionResult SetOverclock(const FString& LocationId, int32 Cell, int32 Milli);
    UFUNCTION(BlueprintPure, Category="Procurement") TArray<FMaiOrderView> GetOrders() const;
    UFUNCTION(BlueprintPure, Category="Procurement") int32 GetStock(const FString& LocationId, EMaiItemKind Kind, int32 Item, bool bGreyOnly) const;
    UFUNCTION(BlueprintCallable, Category="Auction") FMaiActionResult OpenAuction(const FString& LocationId);
    UFUNCTION(BlueprintCallable, Category="Auction") FMaiActionResult Bid(int64 MoneyMicro);
};
