#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MaiCaptureSubsystem.generated.h"
class UMaiNativeWidget;
class UMaiRulesSubsystem;
class FJsonValue;
class FJsonObject;
UCLASS()
class MAKEYOURAI_API UMaiCaptureSubsystem : public UGameInstanceSubsystem, public FTickableGameObject {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return !IsTemplate() && Active; }
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMaiCaptureSubsystem, STATGROUP_Tickables); }
    virtual UWorld* GetTickableGameObjectWorld() const override;
private:
    bool Active=false,Resume=false;
    int32 Stage=0,AfterShot=0;
    double Started=0,Next=0,ShotStarted=0;
    FString Directory,PendingShot,Session;
    TArray<FString> Captures;
    TArray<double> RecentFrameMs;
    bool SweepRequested=false;
    double SweepStarted=0,SweepPreviousTick=0;
    FVector SweepPivot=FVector::ZeroVector,SweepOffset=FVector::ZeroVector;
    TArray<double> SweepFrameMs;
    TSharedPtr<FJsonObject> SweepReport;
    TArray<TSharedPtr<FJsonValue>> PerformanceSamples;
    UPROPERTY(Transient) TObjectPtr<UMaiRulesSubsystem> Rules;
    UMaiNativeWidget* UI() const;
    bool Click(const FString& Id);
    void Capture(const FString& Name,int32 NextStage);
    void Finish(const FString& Error);
};
