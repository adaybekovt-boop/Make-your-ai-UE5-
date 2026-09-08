#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MaiTimeSubsystem.generated.h"
class UMaiCompanySubsystem;
class UMaiRulesSubsystem;

UCLASS()
class MAKEYOURAI_API UMaiTimeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMaiTimeSubsystem, STATGROUP_Tickables); }
    virtual UWorld* GetTickableGameObjectWorld() const override;
    UFUNCTION(BlueprintPure, Category="Time") double GetGameHours() const;
private:
    UPROPERTY(Transient) TObjectPtr<UMaiCompanySubsystem> Company;
    UPROPERTY(Transient) TObjectPtr<UMaiRulesSubsystem> Rules;
    FDelegateHandle ForegroundHandle;
    void EnterForeground();
    double FractionalMicroseconds = 0;
    uint64 SeenGeneration = 0;
    FDelegateHandle BackgroundHandle;
    void EnterBackground();
};
