#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Engine/EngineBaseTypes.h"
#include "MaiLoadingSubsystem.generated.h"
struct FStreamableHandle;
class ULevelStreamingDynamic;
class UMaiCompanySubsystem;

// Lives in GameInstance, not a level/widget. No synthetic progress timer.
UCLASS()
class MAKEYOURAI_API UMaiLoadingSubsystem : public UGameInstanceSubsystem, public FTickableGameObject {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMaiLoadingSubsystem, STATGROUP_Tickables); }
    virtual UWorld* GetTickableGameObjectWorld() const override;
    UFUNCTION(BlueprintCallable, Category="Loading") void Retry();
    // Invalidates callbacks after restoring a campaign, including a saved Loading screen.
    void Invalidate();
    bool CanCancel() const { return !bTravelIssued; }
private:
    UPROPERTY(Transient) TObjectPtr<UMaiCompanySubsystem> Company;

    UPROPERTY(Transient) TObjectPtr<ULevelStreamingDynamic> GarageStream;
    UPROPERTY(Transient) TObjectPtr<ULevelStreamingDynamic> PendingStream;
    TSharedPtr<FStreamableHandle> Assets;
    TArray<FSoftObjectPath> Required;
    uint64 ActiveGeneration=0, CompanyGeneration=0, Serial=0;
    double StartedAt=0;
    bool bAssetsReady=false, bMapRequested=false, bTravelNeeded=false, bTravelIssued=false;
    FString TravelPackage;
    FDelegateHandle TravelFailureHandle;
    void TravelFailed(UWorld* World, ETravelFailure::Type Type, const FString& Error);
    void Start();
    void Fail(const FString& Error);
    void Progress(int32 Bps, const FString& Operation);
};
