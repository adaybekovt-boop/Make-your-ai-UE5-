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
    virtual void Tick(float DeltaSeconds) override;
    void SetRoomBounds(const FVector& Origin, const FVector2D& HalfSize);
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    UFUNCTION(BlueprintCallable, Category="Interaction") void InteractNearest();
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Arm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> GrayboxBody;
    void North(float Value);
    void East(float Value);
    bool CanMoveInCampaign() const;
    FVector RoomOrigin=FVector::ZeroVector;
    FVector2D RoomHalfSize=FVector2D(650,650);
    FVector SafeSpawn=FVector::ZeroVector;
    bool bRoomBoundsSet=false;
};
