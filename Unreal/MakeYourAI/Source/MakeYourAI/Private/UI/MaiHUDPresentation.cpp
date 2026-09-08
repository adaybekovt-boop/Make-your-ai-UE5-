#include "UI/MaiHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Border.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Gameplay/MaiProcurementSubsystem.h"
#include "Persistence/MaiSaveSubsystem.h"
#include "Core/MaiStrings.h"
#include "Engine/GameInstance.h"
#include "String/LexFromString.h"

namespace {
FString Money(mai::Money Value) { return UTF8_TO_TCHAR(mai::FormatMoney(Value).c_str()); }
FString Text(const std::string& Value) { return UTF8_TO_TCHAR(Value.c_str()); }
int32 Selection(const UComboBoxString* Picker) { return Picker ? Picker->FindOptionIndex(Picker->GetSelectedOption()) : -1; }
}
void UMaiHUDWidget::Refresh() {
    if (!CompanyText || !Company) return;
    if (!Company->Domain()) { CompanyText->SetText(Company->LastMessage); return; }
    const auto& Sim = *Company->Domain(); const auto& S = Sim.View(); const auto Rates = Sim.Economy();
    const int64 Minutes = S.now * 60 / mai::Hour + 8 * 60;
    CompanyText->SetText(FText::FromString(FString::Printf(TEXT("%s\nDay %lld / %02lld:%02lld\n%s / %dx\nRevenue %s /h\nExpenses %s /h\nUsers %.2f\nCompute %.2f"), *Money(S.cash), static_cast<long long>(Minutes / 1440 + 1), static_cast<long long>((Minutes / 60) % 24), static_cast<long long>(Minutes % 60), S.paused ? TEXT("PAUSED") : TEXT("RUNNING"), S.speed, *Money(Rates.revenue), *Money(Rates.Expenses()), static_cast<double>(S.usersMicro) / 1000000., static_cast<double>(Rates.computeMilli) / 1000.)));
    const int I = Sim.Definitions().LocationIndex(TCHAR_TO_UTF8(*SelectedLocation));
    if (I >= 0) {
        const auto Index = static_cast<std::size_t>(I); const auto& D = Sim.Definitions().locations[Index]; const auto& L = S.locations[Index];
        const auto Status = Sim.LocationStatus(D.id);
        LocationText->SetText(FText::FromString(FString::Printf(TEXT("%s / %s\nPurchase %s / power %.1f kW\nSelected cell %d:%d"), *Text(D.name), Status == mai::Status::Owned ? TEXT("Owned") : Status == mai::Status::Available ? TEXT("Available") : TEXT("Locked"), *Money(D.price), D.powerWatts / 1000., D.cols ? SelectedCell / D.cols + 1 : 0, D.cols ? SelectedCell % D.cols + 1 : 0)));
        for (int32 Cell = 0; Cell < CellLabels.Num() && static_cast<std::size_t>(Cell) < L.slots.size(); ++Cell) {
            const auto& V = L.slots[static_cast<std::size_t>(Cell)];
            CellLabels[Cell]->SetText(FText::FromString(FString::Printf(TEXT("%s%d:%d\n%s"), Cell == SelectedCell ? TEXT(">") : TEXT(""), Cell / D.cols + 1, Cell % D.cols + 1, V.chassis < 0 ? TEXT("--") : V.chip < 0 ? TEXT("R") : TEXT("ON"))));
        }
        FString Stock;
        for (int J = 0; J < 3; ++J) Stock += FString::Printf(TEXT("%s: %d (%d grey)\n"), *Text(Sim.Definitions().chassis[static_cast<std::size_t>(J)].name), L.chassisStock[static_cast<std::size_t>(J)].total, L.chassisStock[static_cast<std::size_t>(J)].grey);
        for (int J = 0; J < 4; ++J) Stock += FString::Printf(TEXT("%s: %d (%d grey)\n"), *Text(Sim.Definitions().chips[static_cast<std::size_t>(J)].name), L.chipStock[static_cast<std::size_t>(J)].total, L.chipStock[static_cast<std::size_t>(J)].grey);
        InventoryText->SetText(FText::FromString(Stock));
    }
    auto* Procurement = GetGameInstance()->GetSubsystem<UMaiProcurementSubsystem>(); FString Pending;
    if (Procurement) for (const auto& O : Procurement->GetOrders()) Pending += FString::Printf(TEXT("#%lld %s / %s x%d\nETA %.3f game hours / paid %s\n"), static_cast<long long>(O.Id), *O.LocationId, *O.ItemId, O.Quantity, O.RemainingGameHours, *Money(O.PaidMicro));
    OrdersText->SetText(FText::FromString(Pending.IsEmpty() ? TEXT("No orders in transit") : Pending));
    int32 Quantity = 0; const bool ValidQuantity = LexTryParseString(Quantity, *QuantityInput->GetText().ToString()) && Quantity >= 1 && Quantity <= 24;
    const int Rack = Selection(RackPicker), Chip = Selection(ChipPicker), Channel = Selection(ChannelPicker);
    if (ValidQuantity && Rack >= 0 && Rack < 3 && Chip >= 0 && Chip < 4 && Channel >= 0 && Channel < 2) {
        const auto C = static_cast<mai::Channel>(Channel);
        const auto Price = mai::Simulation::OrderPrice(Sim.Definitions().chassis[static_cast<std::size_t>(Rack)].price, C, Quantity) + mai::Simulation::OrderPrice(Sim.Definitions().chips[static_cast<std::size_t>(Chip)].price, C, Quantity);
        QuoteText->SetText(FText::FromString(FString::Printf(TEXT("Kit total %s / discount %d%%\n%s"), *Money(Price), mai::Simulation::DiscountPercent(Quantity), Chip > Sim.Definitions().chassis[static_cast<std::size_t>(Rack)].maxChip ? TEXT("INCOMPATIBLE KIT") : TEXT("Paid once at order time"))));
    } else QuoteText->SetText(FText::FromString(TEXT("Choose valid equipment and quantity")));
    const auto& A = S.auction; const auto& X = Sim.Definitions().extensions;
    const TCHAR* PhaseNames[] = {TEXT("None"), TEXT("Open"), TEXT("Won"), TEXT("Lost")};
    AuctionText->SetText(FText::FromString(FString::Printf(TEXT("%s\nRival: %s\nState: %s / player leading: %s\nBid %s / escrow %s\nTime remaining %.3f game hours"), X.auctionConfigured ? TEXT("Configured auction lot") : TEXT("BALANCE_TUNABLE: configure the catalog first"), *Text(A.rival.empty() ? X.rivalName : A.rival), PhaseNames[static_cast<int>(A.phase)], A.playerLeading ? TEXT("yes") : TEXT("no"), *Money(A.currentBid), *Money(A.escrow), A.phase == mai::AuctionPhase::Open ? static_cast<double>(A.closes - S.now) / mai::Hour : 0.)));
    RegionText->SetText(FText::FromString(FString::Printf(TEXT("Active region: %s\nOne company illegal-data risk: %.2f%% / day\nNuclear: %s\nPlant price: %s / tariff multiplier %.2f%%\nElectricity expense: %s /h"), *Text(S.activeRegion), Sim.CourtRiskPpm() / 10000., S.nuclearOwned ? TEXT("Owned") : X.nuclearConfigured ? TEXT("Available") : TEXT("BALANCE_TUNABLE"), *Money(X.nuclearPrice), X.nuclearTariffBps / 100., *Money(Rates.electricity))));
    FString Notices = ActionMessage.IsEmpty() ? Company->LastMessage.ToString() : ActionMessage.ToString();
    const auto Count = S.notices.size(); for (std::size_t N = Count > 3 ? Count - 3 : 0; N < Count; ++N) Notices += TEXT("\n\n") + Text(S.notices[N]);
    NotificationText->SetText(FText::FromString(Notices));
    if (InventoryCampaignText && Company->Campaign()) {
        FString Datasets;
        for (const auto& B : Company->Campaign()->View().inventory.batches) {
            Datasets += FString::Printf(TEXT("#%lld %s / %s / %s\n"), static_cast<long long>(B.id), *Text(B.offerId), *Text(mai::DatasetStatusName(B.status)), *Money(B.cost));
        }
        InventoryCampaignText->SetText(FText::FromString(Datasets.IsEmpty() ? TEXT("No dataset batches") : Datasets));
    }
    RefreshFlow();
}
void UMaiHUDWidget::RefreshFlow() {
    if (!FlowRoot || !OperationsRoot || !Company || !Company->Campaign()) return;
    const auto& Play = *Company->Campaign();
    const auto Screen = Play.View().screen;
    const bool bOps = Screen == mai::Screen::CityMap || Screen == mai::Screen::Gameplay || Screen == mai::Screen::Training;
    OperationsRoot->SetVisibility(bOps ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    FlowRoot->SetVisibility(bOps && Screen != mai::Screen::Training ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (bOps && Screen != mai::Screen::Training && Screen != mai::Screen::Gameplay) return;
    FString Title = UTF8_TO_TCHAR(mai::ScreenName(Screen).c_str());
    FString Body;
    if (Screen == mai::Screen::Loading) {
        const auto& L = Play.View().loading;
        Body = FString::Printf(TEXT("%s\n%s\n%s"), UTF8_TO_TCHAR(mai::Loc("load.status")), *Text(L.operation), L.progressBps < 0 ? TEXT("Progress: indeterminate") : *FString::Printf(TEXT("Progress: %.0f%%"), L.progressBps / 100.0));
        if (L.phase == mai::LoadPhase::Failed) Body += TEXT("\n") + Text(L.error);
    } else if (Screen == mai::Screen::MainMenu) {
        auto* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiSaveSubsystem>() : nullptr;
        Body = FString::Printf(TEXT("%s\n%s"), UTF8_TO_TCHAR(mai::Loc("app.subtitle")), Saves && SlotInput && Saves->HasSave(SlotInput->GetText().ToString()) ? TEXT("A save exists in the typed slot.") : UTF8_TO_TCHAR(mai::Loc("menu.no-save")));
    } else if (Screen == mai::Screen::NewGame || Screen == mai::Screen::Difficulty) {
        Body = FString::Printf(TEXT("%s\n%s\n%s"), UTF8_TO_TCHAR(mai::Loc("difficulty.easy.blurb")), UTF8_TO_TCHAR(mai::Loc("difficulty.normal.blurb")), UTF8_TO_TCHAR(mai::Loc("difficulty.hard.blurb")));
    } else if (Screen == mai::Screen::Prologue) {
        const char* Key = Play.View().prologueStep == 0 ? "prologue.name-prompt" : Play.View().prologueStep == 1 ? "prologue.budget-prompt" : "prologue.task-prompt";
        Body = UTF8_TO_TCHAR(mai::Loc(Key));
    } else if (Screen == mai::Screen::Training || Screen == mai::Screen::Gameplay) {
        const mai::ReviewSession* Active = nullptr;
        for (const auto& R : Play.View().reviews) if (R.phase == mai::ReviewPhase::Active && R.method == mai::ReviewMethod::Manual) Active = &R;
        if (Active && Active->decisions.size() < Active->items.size()) {
            const auto& Item = Active->items[Active->decisions.size()];
            Title = UTF8_TO_TCHAR(mai::Loc("screen.training"));
            if (ReviewPrompt) ReviewPrompt->SetText(FText::FromString(UTF8_TO_TCHAR(Item.prompt.c_str())));
            if (ReviewLeft) ReviewLeft->SetText(FText::FromString(UTF8_TO_TCHAR(Item.left.c_str())));
            if (ReviewRight) ReviewRight->SetText(FText::FromString(UTF8_TO_TCHAR(Item.right.c_str())));
            Body = FString::Printf(TEXT("Manual review %d / %d"), static_cast<int>(Active->decisions.size() + 1), static_cast<int>(Active->items.size()));
        } else {
            const auto Near = Play.NearbyPoint(150);
            Body = Near ? FString::Printf(TEXT("%s\n%s"), UTF8_TO_TCHAR(mai::Loc("walk.hint")), *Text(Near->action)) : UTF8_TO_TCHAR(mai::Loc("walk.hint"));
        }
    } else if (Screen == mai::Screen::Ending || Screen == mai::Screen::Results) {
        const auto& E = Play.View().ending;
        Body = FString::Printf(TEXT("%s\n%s\n%s\n%s"), *Text(E.title), *Text(E.line),
            E.kind == mai::EndingKind::Acquisition ? UTF8_TO_TCHAR(mai::Loc("ending.elon-max.letter")) : TEXT(""),
            UTF8_TO_TCHAR(mai::Loc("character.elon-max.notice")));
        if (E.offerOnly) Body += TEXT("\n") + FString(UTF8_TO_TCHAR(mai::Loc("ending.offer")));
    } else if (Screen == mai::Screen::Settings) {
        Body = FString::Printf(TEXT("%s: %d%%\n%s: %s"), UTF8_TO_TCHAR(mai::Loc("settings.text-scale")), Play.Settings().textScaleBps / 100,
            UTF8_TO_TCHAR(mai::Loc("settings.reduced-motion")), Play.Settings().reducedMotion ? TEXT("on") : TEXT("off"));
    }
    if (FlowTitle) FlowTitle->SetText(FText::FromString(Title));
    if (FlowBody) FlowBody->SetText(FText::FromString(Body));
}
