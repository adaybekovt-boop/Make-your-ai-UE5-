#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MaiPlayerController.generated.h"
class UMaiHUDWidget;

UCLASS()
class MAKEYOURAI_API AMaiPlayerController : public APlayerController {
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    void SelectLocation(const FString& Id, int32 Cell = -1);
    void EnterLocation(const FString& Id);
    void ShowCity();
    void FocusLandmark(const FString& Id);
    void VisitNpc();
private:
    UPROPERTY(Transient) TObjectPtr<UMaiHUDWidget> Screen;
    void ClickWorld();
};
