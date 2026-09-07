#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MaiSaveGame.generated.h"

UCLASS()
class MAKEYOURAI_API UMaiSaveGame : public USaveGame {
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 FormatVersion = 2;
    UPROPERTY(SaveGame) FString Difficulty;
    UPROPERTY(SaveGame) int32 CurrentScreen = 0;
    UPROPERTY(SaveGame) int32 FirstScreen = 0;
    UPROPERTY(SaveGame) int32 LastScreen = 0;
    UPROPERTY(SaveGame) TArray<uint8> DomainPayload;
};
