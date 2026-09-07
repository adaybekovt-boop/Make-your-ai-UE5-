#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/MaiViewTypes.h"
#include "MaiSaveSubsystem.generated.h"

UCLASS()
class MAKEYOURAI_API UMaiSaveSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Persistence") FMaiActionResult SaveCompany(const FString& SlotName);
    UFUNCTION(BlueprintCallable, Category="Persistence") FMaiActionResult LoadCompany(const FString& SlotName);
    UFUNCTION(BlueprintPure, Category="Persistence") bool HasSave(const FString& SlotName) const;
    static bool IsSafeSlotName(const FString& Name);
};
