#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MaiSaveGame.generated.h"

UCLASS()
class MAKEYOURAI_API UMaiSaveGame : public USaveGame {
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 FormatVersion = 1;
    UPROPERTY(SaveGame) TArray<uint8> DomainPayload;
};
