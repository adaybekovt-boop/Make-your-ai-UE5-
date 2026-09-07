#include "Gameplay/MaiProcurementSubsystem.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Engine/GameInstance.h"

namespace {
FMaiActionResult Execute(UGameInstance* GI, TFunctionRef<mai::Result(mai::Simulation&)> Action) {
    auto* Company = GI ? GI->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    return Company ? Company->Transact(Action) : FMaiActionResult::From(mai::Result::Error("Company subsystem unavailable"));
}
}
FMaiActionResult UMaiProcurementSubsystem::OrderKit(const FString& Id, int32 Rack, int32 Chip, EMaiChannel Channel, int32 Quantity, int32 Cell) {
    return Execute(GetGameInstance(), [&](mai::Simulation& S){return S.OrderKit(TCHAR_TO_UTF8(*Id), Rack, Chip, static_cast<mai::Channel>(Channel), Quantity, Cell);});
}
FMaiActionResult UMaiProcurementSubsystem::OrderItem(const FString& Id, EMaiItemKind Kind, int32 Item, EMaiChannel Channel, int32 Quantity, int32 Cell) {
    return Execute(GetGameInstance(), [&](mai::Simulation& S){return S.OrderItem(TCHAR_TO_UTF8(*Id), static_cast<mai::Kind>(Kind), Item, static_cast<mai::Channel>(Channel), Quantity, Cell);});
}
FMaiActionResult UMaiProcurementSubsystem::MountChassis(const FString& Id, int32 Cell, int32 Rack) {
    return Execute(GetGameInstance(), [&](mai::Simulation& S){return S.MountChassis(TCHAR_TO_UTF8(*Id), Cell, Rack);});
}
FMaiActionResult UMaiProcurementSubsystem::MountChip(const FString& Id, int32 Cell, int32 Chip) {
    return Execute(GetGameInstance(), [&](mai::Simulation& S){return S.MountChip(TCHAR_TO_UTF8(*Id), Cell, Chip);});
}
FMaiActionResult UMaiProcurementSubsystem::SetOverclock(const FString& Id, int32 Cell, int32 Milli) {
    return Execute(GetGameInstance(), [&](mai::Simulation& S){return S.SetOverclock(TCHAR_TO_UTF8(*Id), Cell, Milli);});
}
FMaiActionResult UMaiProcurementSubsystem::OpenAuction(const FString& Id) {
    return Execute(GetGameInstance(), [&](mai::Simulation& S){return S.StartAuction(TCHAR_TO_UTF8(*Id));});
}
FMaiActionResult UMaiProcurementSubsystem::Bid(int64 MoneyMicro) {
    return Execute(GetGameInstance(), [&](mai::Simulation& S){return S.Bid(MoneyMicro);});
}
TArray<FMaiOrderView> UMaiProcurementSubsystem::GetOrders() const {
    TArray<FMaiOrderView> Result;
    const auto* C = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if (!C || !C->Domain()) return Result;
    const auto& S = *C->Domain(); const auto& D = S.Definitions();
    for (const auto& O : S.View().orders) {
        FMaiOrderView V; V.Id = O.id; V.LocationId = UTF8_TO_TCHAR(O.location.c_str());
        const auto& Id = O.kind == mai::Kind::Chassis ? D.chassis[static_cast<std::size_t>(O.item)].id : D.chips[static_cast<std::size_t>(O.item)].id;
        V.ItemId = UTF8_TO_TCHAR(Id.c_str()); V.Quantity = O.quantity; V.Channel = static_cast<EMaiChannel>(O.channel);
        V.PaidMicro = O.paid; V.RemainingGameHours = static_cast<double>(O.arrives - S.View().now) / mai::Hour; Result.Add(V);
    }
    return Result;
}
int32 UMaiProcurementSubsystem::GetStock(const FString& Id, EMaiItemKind Kind, int32 Item, bool bGreyOnly) const {
    const auto* C = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if (!C || !C->Domain() || (Kind != EMaiItemKind::Chassis && Kind != EMaiItemKind::Chip)) return 0;
    const int Index = C->Domain()->Definitions().LocationIndex(TCHAR_TO_UTF8(*Id));
    if (Index < 0 || Item < 0 || Item >= (Kind == EMaiItemKind::Chassis ? 3 : 4)) return 0;
    const auto& L = C->Domain()->View().locations[static_cast<std::size_t>(Index)];
    const auto& Stock = Kind == EMaiItemKind::Chassis ? L.chassisStock[static_cast<std::size_t>(Item)] : L.chipStock[static_cast<std::size_t>(Item)];
    return bGreyOnly ? Stock.grey : Stock.total;
}
