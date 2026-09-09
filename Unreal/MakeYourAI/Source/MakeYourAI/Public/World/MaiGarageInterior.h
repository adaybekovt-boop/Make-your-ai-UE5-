#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MaiGarageInterior.generated.h"
class AMaiInteriorPoint;
UCLASS()
class MAKEYOURAI_API AMaiGarageInterior : public AActor {
    GENERATED_BODY()
public:
    AMaiGarageInterior();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool Build();
    void ConfigureLocation(const FString& Id);
    FVector2D WalkHalfSize() const {return FVector2D(HalfX-50,HalfY-50);}
    FVector PlayerStart() const {return GetActorLocation()+FVector(0,-430,100);}
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Verification") bool bGraybox=true;
private:
    UPROPERTY(Transient) TArray<AActor*> Spawned;
    UPROPERTY(Transient) TArray<AMaiInteriorPoint*> Racks;
    bool bBuilt=false;
    FString LocationId=TEXT("garage");
    float HalfX=700,HalfY=700;
};
