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
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<USpringArmComponent> Arm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<UCameraComponent> Camera;
private:
    void MoveNorth(float Value);
    void MoveEast(float Value);
};
