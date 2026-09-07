#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "MaiProximityComponent.generated.h"

UENUM(BlueprintType)
enum class EMaiNpcState : uint8 { Idle, PhoneCall, React };
UCLASS(BlueprintType)
class MAKEYOURAI_API UMaiProximityEventAsset : public UPrimaryDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC", meta=(ClampMin="1")) float RadiusCm = 500.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC", meta=(ClampMin="1")) int32 StageGameMinutes = 5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC", meta=(ClampMin="2")) int32 CooldownGameMinutes = 45;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC") FText PhoneLine = FText::FromString(TEXT("The delivery is on its way. Check the warehouse when it arrives."));
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC") FText ReactionLine = FText::FromString(TEXT("Hello. Looking for the Garage?"));
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMaiNpcEvent, EMaiNpcState, State, FText, Line);

UCLASS(ClassGroup=(MakeYourAI), meta=(BlueprintSpawnableComponent))
class MAKEYOURAI_API UMaiProximityComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UMaiProximityComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC") TObjectPtr<UMaiProximityEventAsset> Settings;
    UPROPERTY(BlueprintAssignable, Category="NPC") FMaiNpcEvent OnStateChanged;
    UPROPERTY(BlueprintReadOnly, Category="NPC") EMaiNpcState State = EMaiNpcState::Idle;
    UPROPERTY(BlueprintReadOnly, Category="NPC") FText CurrentLine;
};
