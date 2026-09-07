#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/MaiViewTypes.h"
#include "Gameplay/MaiCatalogAsset.h"
#include "Campaign/MaiCampaignAsset.h"
#include "Campaign/MaiCampaign.h"
#include "MaiCompanySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMaiCompanyChanged);
UCLASS()
class MAKEYOURAI_API UMaiCompanySubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    UPROPERTY(BlueprintAssignable, Category="MakeYourAI") FMaiCompanyChanged OnChanged;
    UPROPERTY(BlueprintReadOnly, Category="MakeYourAI") FText LastMessage;
    UFUNCTION(BlueprintPure, Category="MakeYourAI") bool IsReady() const { return Game.IsValid(); }
    UFUNCTION(BlueprintPure, Category="MakeYourAI") int64 GetCashMicro() const;
    UFUNCTION(BlueprintPure, Category="MakeYourAI") EMaiLocationStatus GetLocationStatus(const FString& Id) const;
    UFUNCTION(BlueprintCallable, Category="MakeYourAI") FMaiActionResult NewCompany(int32 Seed);
    UFUNCTION(BlueprintCallable, Category="MakeYourAI") FMaiActionResult BuyLocation(const FString& Id);
    UFUNCTION(BlueprintCallable, Category="MakeYourAI") FMaiActionResult SetPaused(bool bPaused);
    UFUNCTION(BlueprintCallable, Category="MakeYourAI") FMaiActionResult SetSpeed(int32 Speed);
    UFUNCTION(BlueprintPure, Category="MakeYourAI") UMaiCatalogAsset* GetCatalog() const { return Catalog; }
    const mai::Simulation* Domain() const { return Game ? &Game->Core() : nullptr; }
    const mai::Campaign* CampaignDomain() const { return Game.Get(); }
    mai::Campaign* Campaign() { return Game.Get(); }
    const mai::Campaign* Campaign() const { return Game.Get(); }
    UMaiCampaignAsset* CampaignDefinitions() const { return CampaignAsset; }
    FMaiActionResult CampaignTransact(TFunctionRef<mai::Result(mai::Campaign&)> Action);
    FMaiActionResult RunCampaign(TFunctionRef<mai::Result(mai::Campaign&)> Action) { return CampaignTransact(Action); }
    FMaiActionResult Transact(TFunctionRef<mai::Result(mai::Simulation&)> Action);
    FMaiActionResult CompleteHostLoad(const FString& Operation, bool bSuccess = true, const FString& Error = {});
    FMaiActionResult Advance(int64 RealMicroseconds);
    FMaiActionResult LoadPayload(const TArray<uint8>& Bytes);
    TArray<uint8> SavePayload() const;
    bool Proximity(bool bInside, int64 Duration, int64 Cooldown);
    uint64 Generation() const { return CompanyGeneration; }
private:
    UPROPERTY(Transient) TObjectPtr<UMaiCatalogAsset> Catalog;
    UPROPERTY(Transient) TObjectPtr<UMaiCampaignAsset> CampaignAsset;
    TUniquePtr<mai::Campaign> Game;
    uint64 CompanyGeneration = 0;
    void Notify(const mai::Result& Result);
};
