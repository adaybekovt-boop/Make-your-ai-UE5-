#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MaiGameMode.generated.h"

UCLASS()
class MAKEYOURAI_API AMaiGameMode : public AGameModeBase {
    GENERATED_BODY()
public:
    AMaiGameMode();
    virtual void BeginPlay() override;
};
