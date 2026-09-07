#include "Gameplay/MaiCompanySubsystem.h"
#include "Core/MaiGameInstance.h"
#include "Subsystems/SubsystemCollection.h"

void UMaiCompanySubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    auto* Instance=Cast<UMaiGameInstance>(GetGameInstance());
    Catalog=Instance?Instance->ResolveCatalog():NewObject<UMaiCatalogAsset>(this);
    CampaignAsset=Instance?Instance->ResolveCampaign():NewObject<UMaiCampaignAsset>(this);
    NewCompany(Instance?Instance->SessionSeed:1296124209);
}
void UMaiCompanySubsystem::Deinitialize() { Game.Reset(); Catalog=nullptr; CampaignAsset=nullptr; Super::Deinitialize(); }
FMaiActionResult UMaiCompanySubsystem::NewCompany(int32 Seed) {
    FString Error; mai::Catalog Definitions; mai::CampaignRules Rules;
    if (!Catalog || !Catalog->ToDomain(Definitions,Error) || !CampaignAsset || !CampaignAsset->ToDomain(Rules,Error)) {
        LastMessage=FText::FromString(Error.IsEmpty()?TEXT("Company/campaign catalog unavailable"):Error);
        FMaiActionResult Failed; Failed.Message=LastMessage; return Failed;
    }
    Game=MakeUnique<mai::Campaign>(MoveTemp(Definitions),MoveTemp(Rules),static_cast<uint32>(Seed));
    ++CompanyGeneration; LastMessage=FText::FromString(TEXT("Campaign reset; mandatory loading pending")); OnChanged.Broadcast();
    return FMaiActionResult::From(mai::Result::Success("Campaign reset; save slots retained"));
}
int64 UMaiCompanySubsystem::GetCashMicro() const { return Game?Game->Core().View().cash:0; }
EMaiLocationStatus UMaiCompanySubsystem::GetLocationStatus(const FString& Id) const { return Game?static_cast<EMaiLocationStatus>(Game->Core().LocationStatus(TCHAR_TO_UTF8(*Id))):EMaiLocationStatus::Locked; }
FMaiActionResult UMaiCompanySubsystem::CampaignTransact(TFunctionRef<mai::Result(mai::Campaign&)> Action) {
    check(IsInGameThread());
    if (!Game) return FMaiActionResult::From(mai::Result::Error("Campaign unavailable"));
    const auto Before=*Game; auto Result=Action(*Game); std::string Error;
    if (!Game->Validate(Error)) { *Game=Before; Result=mai::Result::Error(Error); }
    auto View=FMaiActionResult::From(Result);
    LastMessage=View.Message.IsEmpty()?FText::FromString(Result.ok?TEXT("Completed"):TEXT("Failed")):View.Message;
    OnChanged.Broadcast(); return View;
}
FMaiActionResult UMaiCompanySubsystem::Transact(TFunctionRef<mai::Result(mai::Simulation&)> Action) {
    if (!Game || !Game->CanPlay()) return FMaiActionResult::From(mai::Result::Error("Finish loading and company setup before gameplay"));
    return CampaignTransact([&](mai::Campaign& C){ return Action(C.Core()); });
}
FMaiActionResult UMaiCompanySubsystem::BuyLocation(const FString& Id) { return Transact([&](mai::Simulation& S){return S.BuyLocation(TCHAR_TO_UTF8(*Id));}); }
FMaiActionResult UMaiCompanySubsystem::SetPaused(bool bPaused) { return CampaignTransact([&](mai::Campaign& C){ C.Core().SetPaused(bPaused);return mai::Result::Success(bPaused?"Simulation paused":"Simulation resumed");}); }
FMaiActionResult UMaiCompanySubsystem::SetSpeed(int32 Speed) { return Transact([&](mai::Simulation& S){return S.SetSpeed(Speed);}); }
FMaiActionResult UMaiCompanySubsystem::Advance(int64 RealMicroseconds) {
    if (!Game) return FMaiActionResult::From(mai::Result::Error("Campaign unavailable"));
    auto Result=Game->AdvanceReal(RealMicroseconds);
    if (!Result.ok) { LastMessage=FText::FromString(UTF8_TO_TCHAR(Result.message.c_str())); Game->Core().SetPaused(true); OnChanged.Broadcast(); }
    return FMaiActionResult::From(Result);
}
TArray<uint8> UMaiCompanySubsystem::SavePayload() const {
    TArray<uint8> Bytes; if (Game) { const auto Text=Game->Save(); Bytes.Append(reinterpret_cast<const uint8*>(Text.data()),static_cast<int32>(Text.size())); } return Bytes;
}
FMaiActionResult UMaiCompanySubsystem::LoadPayload(const TArray<uint8>& Bytes) {
    if (Bytes.Num()==0 || Bytes.Num()>8*1024*1024) return FMaiActionResult::From(mai::Result::Error("Invalid campaign payload size"));
    auto Result=CampaignTransact([&](mai::Campaign& C){return C.Load(std::string(reinterpret_cast<const char*>(Bytes.GetData()),static_cast<std::size_t>(Bytes.Num())));});
    if (Result.bSuccess) ++CompanyGeneration; return Result;
}
bool UMaiCompanySubsystem::Proximity(bool bInside,int64 Duration,int64 Cooldown) { return Game && Game->CanPlay() && Game->Core().Proximity(bInside,Duration,Cooldown); }
