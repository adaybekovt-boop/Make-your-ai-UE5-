#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MaiCameraPawn.generated.h"
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class MAKEYOURAI_API AMaiCameraPawn : public APawn {
    GENERATED_BODY()
public:
    AMaiCameraPawn();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void Focus(const FVector& GroundPosition, float Width);
    void ZoomIn();
    void ZoomOut();
    void RememberOverview();
    void ResetOverview();
    static float WheelZoomFactor(float Delta){return FMath::IsFinite(Delta)?FMath::Pow(.85f,FMath::Clamp(Delta,-8.f,8.f)):1.f;}
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<USpringArmComponent> Arm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<UCameraComponent> Camera;
private:
    bool AllowsInput(bool CheckPointer) const;
    void MouseWheel(float Delta);
    void ResetFromInput();
    void MoveNorth(float Value);
    void MoveEast(float Value);
    void MouseHorizontal(float Value);
    void MouseVertical(float Value);
    void Orbit(float Yaw, float Pitch);
    void Zoom(float Factor);
    FVector GroundTarget() const;
    FTransform Overview;
    bool bHasOverview=false;
};
