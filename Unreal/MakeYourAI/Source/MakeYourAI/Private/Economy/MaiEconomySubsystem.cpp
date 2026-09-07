#include "Economy/MaiEconomySubsystem.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Engine/GameInstance.h"

FMaiEconomyView UMaiEconomySubsystem::GetRates() const {
    const auto* C = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    return C && C->Domain() ? FMaiEconomyView::From(C->Domain()->Economy()) : FMaiEconomyView{};
}
FMaiActionResult UMaiEconomySubsystem::BuyNuclearStation() {
    auto* C = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    return C ? C->Transact([](mai::Simulation& S){return S.BuyNuclear();}) : FMaiActionResult::From(mai::Result::Error("Company unavailable"));
}
