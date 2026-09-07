#include "UI/MaiHUDWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/WidgetSwitcher.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Gameplay/MaiProcurementSubsystem.h"
#include "Gameplay/MaiRegionSubsystem.h"
#include "Economy/MaiEconomySubsystem.h"
#include "Persistence/MaiSaveSubsystem.h"
#include "World/MaiPlayerController.h"
#include "Campaign/MaiCampaign.h"
#include "Core/MaiStrings.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/LexFromString.h"

void UMaiHUDWidget::LocationChanged(FString Value, ESelectInfo::Type Type) {
    (void)Type; if (Value.IsEmpty()) return;
    SelectedLocation = Value; SelectedCell = 0; RebuildGrid(); Refresh();
}
void UMaiHUDWidget::PageChanged(FString Value, ESelectInfo::Type Type) {
    (void)Type; if (Pages && PagePicker) Pages->SetActiveWidgetIndex(FMath::Max(0, PagePicker->FindOptionIndex(Value)));
}
void UMaiHUDWidget::SelectLocation(const FString& Id, int32 Cell) {
    if (!PagePicker) return;
    if (Id == TEXT("auction")) { PagePicker->SetSelectedIndex(5); return; }
    if (Id == TEXT("nuclear-power") || Id == TEXT("greenhaven")) { PagePicker->SetSelectedIndex(6); if (Id == TEXT("greenhaven")) RegionPicker->SetSelectedOption(TEXT("greenhaven")); return; }
    if (!Company || !Company->Domain() || Company->Domain()->Definitions().LocationIndex(TCHAR_TO_UTF8(*Id)) < 0) {
        ActionMessage = FText::FromString(TEXT("This landmark has no gameplay operation in the current vertical slice.")); Refresh(); return;
    }
    LocationPicker->SetSelectedOption(Id); SelectedLocation = Id;
    if (Cell >= 0) SelectedCell = Cell;
    PagePicker->SetSelectedIndex(Cell >= 0 ? 1 : 0); RebuildGrid(); Refresh();
}
void UMaiHUDWidget::ShowResult(const FMaiActionResult& Result) {
    ActionMessage = Result.Message.IsEmpty() ? FText::FromString(Result.bSuccess ? TEXT("Completed") : TEXT("Failed")) : Result.Message;
}
void UMaiHUDWidget::HandleCommand(const FString& Command) {
    if (!Company || !Company->Domain()) return;
    auto* GI = GetGameInstance(); auto* P = GI->GetSubsystem<UMaiProcurementSubsystem>(); auto* PC = Cast<AMaiPlayerController>(GetOwningPlayer());
    if (!P) return;
    int32 Quantity = 0; LexTryParseString(Quantity, *QuantityInput->GetText().ToString());
    const int32 Rack = RackPicker->FindOptionIndex(RackPicker->GetSelectedOption()), Chip = ChipPicker->FindOptionIndex(ChipPicker->GetSelectedOption());
    const auto Channel = static_cast<EMaiChannel>(ChannelPicker->FindOptionIndex(ChannelPicker->GetSelectedOption()));
    if (Command != TEXT("new")) bNewCompanyArmed = false;
    if (Command.StartsWith(TEXT("cell:"))) { int32 Cell = 0; if (LexTryParseString(Cell, *Command.Mid(5))) SelectedCell = Cell; }
    else if (Command == TEXT("pause")) ShowResult(Company->SetPaused(!Company->Domain()->View().paused));
    else if (Command == TEXT("speed1") || Command == TEXT("speed3")) ShowResult(Company->SetSpeed(Command == TEXT("speed1") ? 1 : 3));
    else if (Command == TEXT("buy")) ShowResult(Company->BuyLocation(SelectedLocation));
    else if (Command == TEXT("kit")) ShowResult(P->OrderKit(SelectedLocation, Rack, Chip, Channel, Quantity, SelectedCell));
    else if (Command == TEXT("rackorder")) ShowResult(P->OrderItem(SelectedLocation, EMaiItemKind::Chassis, Rack, Channel, Quantity, SelectedCell));
    else if (Command == TEXT("chiporder") || Command == TEXT("upgradeorder")) ShowResult(P->OrderItem(SelectedLocation, EMaiItemKind::Chip, Chip, Channel, Quantity, Command == TEXT("upgradeorder") ? SelectedCell : -1));
    else if (Command == TEXT("mountrack")) ShowResult(P->MountChassis(SelectedLocation, SelectedCell, Rack));
    else if (Command == TEXT("mountchip")) ShowResult(P->MountChip(SelectedLocation, SelectedCell, Chip));
    else if (Command == TEXT("save")) ShowResult(GI->GetSubsystem<UMaiSaveSubsystem>()->SaveCompany(SlotInput->GetText().ToString()));
    else if (Command == TEXT("load")) { ShowResult(GI->GetSubsystem<UMaiSaveSubsystem>()->LoadCompany(SlotInput->GetText().ToString())); RebuildGrid(); }
    else if (Command == TEXT("auctionopen")) ShowResult(P->OpenAuction(SelectedLocation));
    else if (Command == TEXT("bid")) {
        int64 Dollars = 0;
        if (!LexTryParseString(Dollars, *BidInput->GetText().ToString()) || Dollars <= 0 || Dollars > 1000000) ActionMessage = FText::FromString(TEXT("Bid must be 1-1000000 whole dollars"));
        else ShowResult(P->Bid(mai::Dollars(Dollars)));
    }
    else if (Command == TEXT("unlockregion")) ShowResult(GI->GetSubsystem<UMaiRegionSubsystem>()->Unlock(RegionPicker->GetSelectedOption()));
    else if (Command == TEXT("switchregion")) ShowResult(GI->GetSubsystem<UMaiRegionSubsystem>()->SwitchTo(RegionPicker->GetSelectedOption()));
    else if (Command == TEXT("nuclear")) ShowResult(GI->GetSubsystem<UMaiEconomySubsystem>()->BuyNuclearStation());
    else if (Command == TEXT("procurepage")) PagePicker->SetSelectedIndex(1);
    else if (Command == TEXT("city") && PC) PC->ShowCity();
    else if (Command == TEXT("focus") && PC) PC->FocusLandmark(SelectedLocation);
    else if (Command == TEXT("enter") && PC) {
        if (Company->GetLocationStatus(SelectedLocation) != EMaiLocationStatus::Owned) ActionMessage = FText::FromString(TEXT("Buy the location before entering"));
        else PC->EnterLocation(SelectedLocation);
    }
    else if (Command == TEXT("npc") && PC) PC->VisitNpc();
    else if (Command == TEXT("new") || Command == TEXT("menu-new")) {
        if (!bNewCompanyArmed) { bNewCompanyArmed = true; ActionMessage = FText::FromString(TEXT("Press New company again to discard the current unsaved session. Save slots are not deleted.")); }
        else { bNewCompanyArmed = false; ShowResult(Company->CampaignTransact([](mai::Campaign& G){ return G.ShowScreen(mai::Screen::MainMenu); })); RebuildGrid(); }
    }
    else if (Command == TEXT("menu-load")) {
        auto* Saves = GI->GetSubsystem<UMaiSaveSubsystem>();
        if (!Saves || !Saves->HasSave(SlotInput->GetText().ToString())) ActionMessage = FText::FromString(UTF8_TO_TCHAR(mai::Loc("menu.no-save")));
        else { ShowResult(Saves->LoadCompany(SlotInput->GetText().ToString())); RebuildGrid(); }
    }
    else if (Command == TEXT("menu-settings")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.ShowScreen(mai::Screen::Settings); }));
    else if (Command == TEXT("menu-back")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.ShowScreen(mai::Screen::MainMenu); }));
    else if (Command == TEXT("menu-quit")) {
        ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.RequestQuit(); }));
        if (Company->Campaign() && Company->Campaign()->WantsQuit()) UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
    }
    else if (Command == TEXT("diff-Easy") || Command == TEXT("diff-Normal") || Command == TEXT("diff-Hard")) {
        const FString Id = Command.Mid(5);
        ShowResult(Company->RunCampaign([&](mai::Campaign& C) {
            if (C.View().screen == mai::Screen::NewGame) { auto R = C.ShowDifficulty(); if (!R.ok) return R; }
            return C.ChooseDifficulty(TCHAR_TO_UTF8(*Id));
        }));
    }
    else if (Command == TEXT("prologue-next") && Company->Campaign()) {
        const auto Step = Company->Campaign()->View().prologueStep;
        if (Step == 0) ShowResult(Company->RunCampaign([&](mai::Campaign& C){ return C.PrologueAction(TCHAR_TO_UTF8(*CompanyNameInput->GetText().ToString())); }));
        else if (Step == 1) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.PrologueAction("budget"); }));
        else {
            ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.PrologueAction("accept-task"); }));
            Company->CompleteHostLoad(TEXT("City map after prologue"));
        }
    }
    else if (Command == TEXT("load-retry")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.RetryLoad(); }));
    else if (Command == TEXT("load-cancel")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.CancelLoad(); }));
    else if (Command == TEXT("walk") && PC) PC->EnterLocation(TEXT("garage"));
    else if (Command == TEXT("buy-text")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.BuyDataset("official-text-10"); }));
    else if (Command == TEXT("buy-image")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.BuyDataset("official-image-10"); }));
    else if (Command == TEXT("buy-mixed")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.BuyDataset("official-mixed-10"); }));
    else if (Command == TEXT("hire")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.HireSpecialist(); }));
    else if (Command == TEXT("ai-create")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.CreateAIReviewer(); }));
    else if (Command == TEXT("review-manual") || Command == TEXT("review-human") || Command == TEXT("review-ai")) {
        const mai::ReviewMethod Method = Command.EndsWith(TEXT("manual")) ? mai::ReviewMethod::Manual : Command.EndsWith(TEXT("human")) ? mai::ReviewMethod::Human : mai::ReviewMethod::AI;
        ShowResult(Company->RunCampaign([&](mai::Campaign& C) {
            for (const auto& B : C.View().inventory.batches) if (B.status == mai::DatasetStatus::Unreviewed) return C.StartReview(B.id, Method);
            return mai::Result::Error("No Unreviewed batch");
        }));
    }
    else if (Command == TEXT("train")) {
        ShowResult(Company->RunCampaign([](mai::Campaign& C) {
            for (const auto& B : C.View().inventory.batches) if (B.status == mai::DatasetStatus::Verified) return C.StartTraining(B.id);
            return mai::Result::Error("No Verified batch");
        }));
    }
    else if (Command.StartsWith(TEXT("review-")) && Company->Campaign()) {
        std::int64_t Session = 0;
        for (const auto& R : Company->Campaign()->View().reviews) if (R.phase == mai::ReviewPhase::Active && R.method == mai::ReviewMethod::Manual) Session = R.id;
        if (Command == TEXT("review-cancel")) ShowResult(Company->RunCampaign([&](mai::Campaign& C){ return C.CancelReview(Session); }));
        else if (Command == TEXT("review-skip")) ShowResult(Company->RunCampaign([&](mai::Campaign& C){ return C.SkipReview(Session); }));
        else {
            const int Side = Command == TEXT("review-left") ? 0 : Command == TEXT("review-right") ? 1 : 2;
            ShowResult(Company->RunCampaign([&](mai::Campaign& C){ return C.ChooseReview(Session, Side); }));
        }
    }
    else if (Command == TEXT("ending-accept")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.ChooseEnding(mai::EndingKind::Acquisition); }));
    else if (Command == TEXT("ending-decline")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.DeclineSale(); }));
    else if (Command == TEXT("ending-open")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.ChooseEnding(mai::EndingKind::OpenModel); }));
    else if (Command == TEXT("ending-ack")) ShowResult(Company->RunCampaign([](mai::Campaign& C){ return C.AcknowledgeEnding(); }));
    Refresh();
}

void UMaiHUDWidget::ShowWarehouse() { SelectLocation(TEXT("garage")); if(PagePicker) PagePicker->SetSelectedIndex(2); }
