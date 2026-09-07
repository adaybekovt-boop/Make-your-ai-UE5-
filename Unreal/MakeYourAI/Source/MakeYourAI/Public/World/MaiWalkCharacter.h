#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MaiWalkCharacter.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
UCLASS()
class MAKEYOURAI_API AMaiWalkCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AMaiWalkCharacter();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    UFUNCTION(BlueprintCallable, Category="Interaction") void InteractNearest();
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Arm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> GrayboxBody;
    void North(float Value);
    void East(float Value);
    bool CanMoveInCampaign() const;
};
