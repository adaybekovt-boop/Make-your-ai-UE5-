#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "Campaign/MaiCampaign.h"
#include "Rules/MaiRulesVM.h"
#include "MaiRulesSubsystem.generated.h"
class UMaiCompanySubsystem;
UCLASS()
class MAKEYOURAI_API UMaiRulesSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool IsReady() const { return VM&&VM->IsReady(); }
    bool OwnsSimulation() const { return true; } // fail closed; never fall back to the second economy
    void AdvanceFrame(float Seconds);
    void SetForeground(bool Foreground);
    bool Dispatch(const FString& Action,const TArray<TSharedPtr<FJsonValue>>& Args={});
    TSharedPtr<FJsonObject> ViewModel();
    TSharedPtr<FJsonObject> CanonicalState() const { return State; }
    bool SaveSlot(const FString& Slot=TEXT("campaign"));
    bool LoadSlot(const FString& Slot=TEXT("campaign"));
    FString LastError;
    bool bReviewOpen=false;
private:
    UPROPERTY(Transient) TObjectPtr<UMaiCompanySubsystem> Company;
    TUniquePtr<mai::RulesVM> VM;
    TSharedPtr<FJsonObject> State;
    TMap<FString,int64> ReviewLinks;
    TSharedPtr<FJsonObject> RestoreRollback;
    float FrameCarry=0,AutoSaveCarry=0;
    bool bForeground=true,bSkipForegroundFrame=false,bHasSave=false,bLoadingTransaction=false,bRestoreCancelled=false;
    float Volume=.8f,InterfaceScale=1.f;
    bool Invoke(const TSharedRef<FJsonObject>& Request,TSharedPtr<FJsonObject>& Value);
    bool Command(const FString& Action,const TArray<TSharedPtr<FJsonValue>>& Args);
    bool Project(double ElapsedHours=0);
    bool SyncReviews();
    bool NativeAction(TFunctionRef<mai::Result(mai::Campaign&)> Action);
    bool HostAction(const FString& Action,const TArray<TSharedPtr<FJsonValue>>& Args);
    TSharedPtr<FJsonObject> HostView();
    TSharedPtr<FJsonObject> ReviewView() const;
    TSharedPtr<FJsonObject> Capture();
    bool ApplyCapture(const TSharedPtr<FJsonObject>& Capture,bool ValidateOnly=false);
    void CheckRestore();
    FString SlotPath(const FString& Slot) const;
};
