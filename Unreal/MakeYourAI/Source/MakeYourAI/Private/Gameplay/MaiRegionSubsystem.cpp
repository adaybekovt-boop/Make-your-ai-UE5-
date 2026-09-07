#include "Gameplay/MaiRegionSubsystem.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Engine/GameInstance.h"

FMaiActionResult UMaiRegionSubsystem::Unlock(const FString& Id) {
    auto* C = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    return C ? C->Transact([&](mai::Simulation& S){return S.UnlockRegion(TCHAR_TO_UTF8(*Id));}) : FMaiActionResult::From(mai::Result::Error("Company unavailable"));
}
FMaiActionResult UMaiRegionSubsystem::SwitchTo(const FString& Id) {
    auto* C = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    return C ? C->Transact([&](mai::Simulation& S){return S.SwitchRegion(TCHAR_TO_UTF8(*Id));}) : FMaiActionResult::From(mai::Result::Error("Company unavailable"));
}
FString UMaiRegionSubsystem::GetActiveRegion() const {
    const auto* C = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    return C && C->Domain() ? UTF8_TO_TCHAR(C->Domain()->View().activeRegion.c_str()) : FString();
}
int32 UMaiRegionSubsystem::GetCompanyCourtRiskPpm() const {
    const auto* C = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    return C && C->Domain() ? C->Domain()->CourtRiskPpm() : 0;
}
