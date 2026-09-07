#include "Gameplay/MaiCompanySubsystem.h"
#include "Core/MaiGameInstance.h"
#include "Subsystems/SubsystemCollection.h"

void UMaiCompanySubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    auto* Instance = Cast<UMaiGameInstance>(GetGameInstance());
    Catalog = Instance ? Instance->ResolveCatalog() : NewObject<UMaiCatalogAsset>(this);
    NewCompany(Instance ? Instance->SessionSeed : 1296124209);
}
void UMaiCompanySubsystem::Deinitialize() { Sim.Reset(); Catalog = nullptr; Super::Deinitialize(); }
FMaiActionResult UMaiCompanySubsystem::NewCompany(int32 Seed) {
    FString Error; mai::Catalog Definitions;
    if (!Catalog || !Catalog->ToDomain(Definitions, Error)) {
        LastMessage = FText::FromString(Catalog ? Error : TEXT("No configured catalog"));
        FMaiActionResult Failed; Failed.Message = LastMessage; return Failed;
    }
    Sim = MakeUnique<mai::Simulation>(MoveTemp(Definitions), static_cast<uint32>(Seed));
    ++CompanyGeneration; LastMessage = FText::FromString(TEXT("New company. SOURCE_SCAFFOLD.")); OnChanged.Broadcast();
    return FMaiActionResult::From(mai::Result::Success("New company created"));
}
int64 UMaiCompanySubsystem::GetCashMicro() const { return Sim ? Sim->View().cash : 0; }
EMaiLocationStatus UMaiCompanySubsystem::GetLocationStatus(const FString& Id) const {
    return Sim ? static_cast<EMaiLocationStatus>(Sim->LocationStatus(TCHAR_TO_UTF8(*Id))) : EMaiLocationStatus::Locked;
}
FMaiActionResult UMaiCompanySubsystem::Transact(TFunctionRef<mai::Result(mai::Simulation&)> Action) {
    check(IsInGameThread());
    if (!Sim) return FMaiActionResult::From(mai::Result::Error("Company unavailable; inspect catalog errors"));
    const auto Before = Sim->View();
    auto Result = Action(*Sim);
    std::string Error;
    if (Result.ok && !Sim->Validate(Sim->View(), Error)) { Sim->Restore(Before); Result = mai::Result::Error(Error); }
    auto View = FMaiActionResult::From(Result);
    LastMessage = View.Message.IsEmpty() ? FText::FromString(Result.ok ? TEXT("Action completed") : TEXT("Action failed")) : View.Message;
    OnChanged.Broadcast(); return View;
}
FMaiActionResult UMaiCompanySubsystem::BuyLocation(const FString& Id) { return Transact([&](mai::Simulation& S){ return S.BuyLocation(TCHAR_TO_UTF8(*Id)); }); }
FMaiActionResult UMaiCompanySubsystem::SetPaused(bool bPaused) { return Transact([&](mai::Simulation& S){ S.SetPaused(bPaused); return mai::Result::Success(bPaused ? "Simulation paused" : "Simulation resumed"); }); }
FMaiActionResult UMaiCompanySubsystem::SetSpeed(int32 Speed) { return Transact([&](mai::Simulation& S){ return S.SetSpeed(Speed); }); }
FMaiActionResult UMaiCompanySubsystem::Advance(int64 RealMicroseconds) {
    if (!Sim) return FMaiActionResult::From(mai::Result::Error("Company unavailable"));
    auto Result = Sim->AdvanceReal(RealMicroseconds);
    if (!Result.ok) { LastMessage = FText::FromString(UTF8_TO_TCHAR(Result.message.c_str())); Sim->SetPaused(true); OnChanged.Broadcast(); }
    return FMaiActionResult::From(Result);
}
TArray<uint8> UMaiCompanySubsystem::SavePayload() const {
    TArray<uint8> Bytes;
    if (Sim) { const auto Text = Sim->Save(); Bytes.Append(reinterpret_cast<const uint8*>(Text.data()), static_cast<int32>(Text.size())); }
    return Bytes;
}
FMaiActionResult UMaiCompanySubsystem::LoadPayload(const TArray<uint8>& Bytes) {
    if (Bytes.Num() == 0 || Bytes.Num() > 1024 * 1024) return FMaiActionResult::From(mai::Result::Error("Invalid save payload size"));
    auto Result = Transact([&](mai::Simulation& S){ return S.Load(std::string(reinterpret_cast<const char*>(Bytes.GetData()), static_cast<std::size_t>(Bytes.Num()))); });
    if (Result.bSuccess) ++CompanyGeneration; return Result;
}
bool UMaiCompanySubsystem::Proximity(bool bInside, int64 Duration, int64 Cooldown) {
    return Sim && Sim->Proximity(bInside, Duration, Cooldown);
}
