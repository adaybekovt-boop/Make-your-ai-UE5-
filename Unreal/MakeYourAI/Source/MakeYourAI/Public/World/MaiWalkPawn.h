#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MaiWalkPawn.generated.h"
class UCapsuleComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class UStaticMeshComponent;

// Temporary controllable pawn. Interfaces stay so a skeletal character can replace
// this proxy. This is not an imported Garage interior or animated person.
UCLASS()
class MAKEYOURAI_API AMaiWalkPawn : public APawn {
    GENERATED_BODY()
public:
    AMaiWalkPawn();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Tick(float DeltaSeconds) override;
    void Interact();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Walk") TObjectPtr<UCapsuleComponent> Capsule;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Walk") TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Walk") TObjectPtr<UFloatingPawnMovement> Movement;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Walk") TObjectPtr<UStaticMeshComponent> ProxyBody;
private:
    void MoveForward(float Value);
    void MoveRight(float Value);
};
