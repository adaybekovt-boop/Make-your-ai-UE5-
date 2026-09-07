#include "Gameplay/MaiCatalogAsset.h"

UMaiCatalogAsset::UMaiCatalogAsset() {
    const auto Default = mai::Catalog::Defaults();
    for (const auto& D : Default.chips) {
        FMaiChipDefinition V; V.Id = UTF8_TO_TCHAR(D.id.c_str()); V.DisplayName = UTF8_TO_TCHAR(D.name.c_str());
        V.PriceMicro = D.price; V.MaintenanceMicro = D.maintenance; V.PowerWatts = D.watts; V.ComputeMilli = D.computeMilli;
        V.OfficialHours = D.deliveryHours[0]; V.GreyHours = D.deliveryHours[1]; Chips.Add(V);
    }
    for (const auto& D : Default.chassis) {
        FMaiChassisDefinition V; V.Id = UTF8_TO_TCHAR(D.id.c_str()); V.DisplayName = UTF8_TO_TCHAR(D.name.c_str());
        V.PriceMicro = D.price; V.MaximumChipIndex = D.maxChip; V.FailureMultiplierBps = D.failureBps;
        V.ComputeMultiplierBps = D.computeBps; V.OfficialHours = D.deliveryHours[0]; V.GreyHours = D.deliveryHours[1]; Chassis.Add(V);
    }
    for (const auto& D : Default.locations) {
        FMaiLocationDefinition V; V.Id = UTF8_TO_TCHAR(D.id.c_str()); V.DisplayName = UTF8_TO_TCHAR(D.name.c_str());
        V.RegionId = UTF8_TO_TCHAR(D.region.c_str()); V.PriceMicro = D.price; V.RentMicro = D.rent;
        V.Rows = D.rows; V.Columns = D.cols; V.PowerWatts = D.powerWatts; V.bConfigured = D.configured; Locations.Add(V);
    }
    for (const auto& D : Default.regions) {
        FMaiRegionDefinition V; V.Id = UTF8_TO_TCHAR(D.id.c_str()); V.DisplayName = UTF8_TO_TCHAR(D.name.c_str());
        V.UnlockPriceMicro = D.unlockPrice; V.ElectricityMultiplierBps = D.tariffBps; V.CourtRiskMultiplierBps = D.courtBps;
        V.bConfigured = D.configured; V.bBalanceTunable = D.balanceTunable; Regions.Add(V);
    }
}
bool UMaiCatalogAsset::ToDomain(mai::Catalog& Out, FString& Error) const {
    if (Chips.Num() != 4 || Chassis.Num() != 3) { Error = TEXT("Catalog requires four chips and three chassis in stable ID order"); return false; }
    Out = mai::Catalog::Defaults();
    for (int32 I = 0; I < Chips.Num(); ++I) {
        const auto& V = Chips[I]; auto& D = Out.chips[static_cast<std::size_t>(I)];
        if (V.Id != UTF8_TO_TCHAR(D.id.c_str())) { Error = TEXT("Chip IDs cannot be reordered without migration"); return false; }
        D.name = TCHAR_TO_UTF8(*V.DisplayName); D.price = V.PriceMicro; D.maintenance = V.MaintenanceMicro;
        D.watts = V.PowerWatts; D.computeMilli = V.ComputeMilli; D.deliveryHours = {V.OfficialHours, V.GreyHours};
    }
    for (int32 I = 0; I < Chassis.Num(); ++I) {
        const auto& V = Chassis[I]; auto& D = Out.chassis[static_cast<std::size_t>(I)];
        if (V.Id != UTF8_TO_TCHAR(D.id.c_str())) { Error = TEXT("Chassis IDs cannot be reordered without migration"); return false; }
        D.name = TCHAR_TO_UTF8(*V.DisplayName); D.price = V.PriceMicro; D.maxChip = V.MaximumChipIndex;
        D.failureBps = V.FailureMultiplierBps; D.computeBps = V.ComputeMultiplierBps; D.deliveryHours = {V.OfficialHours, V.GreyHours};
    }
    Out.locations.clear();
    for (const auto& V : Locations) Out.locations.push_back({TCHAR_TO_UTF8(*V.Id), TCHAR_TO_UTF8(*V.DisplayName), TCHAR_TO_UTF8(*V.RegionId), V.PriceMicro, V.RentMicro, V.Rows, V.Columns, V.PowerWatts, V.bConfigured});
    Out.regions.clear();
    for (const auto& V : Regions) Out.regions.push_back({TCHAR_TO_UTF8(*V.Id), TCHAR_TO_UTF8(*V.DisplayName), V.UnlockPriceMicro, V.ElectricityMultiplierBps, V.CourtRiskMultiplierBps, V.bConfigured, V.bBalanceTunable});
    auto& X = Out.extensions;
    X.nuclearConfigured = Extensions.bNuclearConfigured; X.nuclearPrice = Extensions.NuclearPriceMicro; X.nuclearTariffBps = Extensions.NuclearTariffMultiplierBps;
    X.auctionConfigured = Extensions.bAuctionConfigured; X.rivalName = TCHAR_TO_UTF8(*Extensions.RivalName);
    X.auctionReserve = Extensions.ReserveMicro; X.bidIncrement = Extensions.IncrementMicro; X.rivalBudget = Extensions.RivalBudgetMicro;
    X.auctionDuration = static_cast<mai::Tick>(Extensions.DurationGameMinutes) * mai::Hour / 60;
    X.rivalInterval = static_cast<mai::Tick>(Extensions.RivalIntervalGameMinutes) * mai::Hour / 60;
    X.rivalAggressionPpm = Extensions.RivalAggressionPpm; X.auctionRack = Extensions.ChassisIndex;
    X.auctionChip = Extensions.ChipIndex; X.auctionQty = Extensions.Quantity; X.auctionChannel = static_cast<mai::Channel>(Extensions.Channel);
    Out.electricityPerKwh = ElectricityPerKwhMicro; Out.defectPpm = GreyDefectPpm; Out.failurePpm = DailyFailurePpm;
    std::string Why; const bool Valid = Out.Valid(Why); Error = UTF8_TO_TCHAR(Why.c_str()); return Valid;
}
