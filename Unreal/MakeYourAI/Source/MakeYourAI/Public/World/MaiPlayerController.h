#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MaiPlayerController.generated.h"
class UMaiHUDWidget;
class UMaiFlowWidget;
class UMaiNativeWidget;
class AMaiCameraPawn;
class AMaiWalkCharacter;
class AMaiWalkPawn;
class AMaiGarageInterior;

UCLASS()
class MAKEYOURAI_API AMaiPlayerController : public APlayerController {
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    UMaiNativeWidget* NativeUI() const { return NativeScreen; }
    void SelectLocation(const FString& Id, int32 Cell = -1);
    void EnterLocation(const FString& Id);
    void ShowCity();
    void ShowWarehouse();
    void FocusLandmark(const FString& Id);
    void VisitNpc();
    void OpenTraining(bool bFromDesk = false);
    bool AtReviewDesk() const;
    bool PrepareCampaignScene(const FString& Interior, bool bMenu, FString& Error);
    void ToggleOperations() { bOperationsOpen = !bOperationsOpen; }
private:
    UPROPERTY(Transient) TObjectPtr<UMaiNativeWidget> NativeScreen;
    UPROPERTY(Transient) TObjectPtr<UMaiHUDWidget> Screen;
    UPROPERTY(Transient) TObjectPtr<UMaiFlowWidget> Flow;
    UPROPERTY(Transient) TObjectPtr<AMaiCameraPawn> CityCamera;
    UPROPERTY(Transient) TObjectPtr<AMaiWalkCharacter> Walker;
    UPROPERTY(Transient) TObjectPtr<AMaiWalkPawn> WalkPawn;
    UPROPERTY(Transient) TObjectPtr<AMaiGarageInterior> RuntimeGarage;
    bool bOperationsOpen = true;
    float QualityClock=0;
    bool bDistantView=false;
    void ClickWorld();
    void PossessCity();
    void PossessWalk();
};
