#include "UI/MaiHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UMaiActionButton::Configure(UMaiHUDWidget* Owner, const FString& InCommand) {
    Screen = Owner; Command = InCommand; OnClicked.AddUniqueDynamic(this, &UMaiActionButton::Execute);
}
void UMaiActionButton::Execute() { if (Screen) Screen->HandleCommand(Command); }
TSharedRef<SWidget> UMaiHUDWidget::RebuildWidget() {
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("NativeWidgetTree"));
    if (!WidgetTree->RootWidget) BuildLayout();
    return Super::RebuildWidget();
}
UTextBlock* UMaiHUDWidget::AddText(UPanelWidget* Parent, const FString& Value, int32 FontSize) {
    auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(FText::FromString(Value)); Label->SetAutoWrapText(true);
    auto Font = Label->GetFont(); Font.Size = FontSize; Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.88f, 0.9f, 1)));
    if (Parent) Parent->AddChild(Label); return Label;
}
UMaiActionButton* UMaiHUDWidget::AddButton(UPanelWidget* Parent, const FString& Value, const FString& Command) {
    auto* Button = WidgetTree->ConstructWidget<UMaiActionButton>(); Button->Configure(this, Command);
    Button->SetBackgroundColor(FLinearColor(0.17f, 0.17f, 0.19f, 1));
    auto* Padding = WidgetTree->ConstructWidget<UBorder>(); Padding->SetPadding(FMargin(8, 6)); Padding->SetBrushColor(FLinearColor::Transparent);
    Padding->SetContent(AddText(nullptr, Value)); Button->SetContent(Padding);
    if (Parent) Parent->AddChild(Button); return Button;
}
UComboBoxString* UMaiHUDWidget::AddPicker(UPanelWidget* Parent, const TArray<FString>& Options) {
    auto* Picker = WidgetTree->ConstructWidget<UComboBoxString>();
    for (const auto& Option : Options) Picker->AddOption(Option);
    if (Options.Num()) Picker->SetSelectedOption(Options[0]); Parent->AddChild(Picker); return Picker;
}
UEditableTextBox* UMaiHUDWidget::AddInput(UPanelWidget* Parent, const FString& Default) {
    auto* Input = WidgetTree->ConstructWidget<UEditableTextBox>(); Input->SetText(FText::FromString(Default)); Parent->AddChild(Input); return Input;
}
void UMaiHUDWidget::BuildLayout() {
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Canvas;
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    const auto Panel = [&](bool Right, float Width) {
        auto* Border = WidgetTree->ConstructWidget<UBorder>(); Border->SetPadding(FMargin(16)); Border->SetBrushColor(FLinearColor(0.025f, 0.025f, 0.03f, 0.97f));
        auto* Slot = Canvas->AddChildToCanvas(Border); Slot->SetAnchors(Right ? FAnchors(1, 0, 1, 1) : FAnchors(0, 0, 0, 1));
        Slot->SetOffsets(Right ? FMargin(-Width - 18, 18, Width, 18) : FMargin(18, 18, Width, 18));
        auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(); Border->SetContent(Scroll);
        auto* Box = WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Box); return Box;
    };
    auto* Left = Panel(false, 275);
    AddText(Left, TEXT("NEURON"), 28); AddText(Left, TEXT("MAKE YOUR AI / SOURCE_SCAFFOLD"), 11);
    CompanyText = AddText(Left, TEXT("Loading company..."), 16);
    auto* TimeRow = WidgetTree->ConstructWidget<UHorizontalBox>(); Left->AddChild(TimeRow);
    AddButton(TimeRow, TEXT("Pause"), TEXT("pause")); AddButton(TimeRow, TEXT("1x"), TEXT("speed1")); AddButton(TimeRow, TEXT("3x"), TEXT("speed3"));
    AddButton(Left, TEXT("City map"), TEXT("city")); AddButton(Left, TEXT("Focus selected location"), TEXT("focus"));
    AddButton(Left, TEXT("Enter owned location"), TEXT("enter")); AddButton(Left, TEXT("Visit Garage contact"), TEXT("npc"));
    AddText(Left, TEXT("WASD: pan / wheel: zoom. Click a marker or grid cell. Pause freezes company time, not the UI."), 12);
    NotificationText = AddText(Left, TEXT(""), 13);
    AddText(Left, TEXT("UE compilation, Editor, visual and gameplay verification: NOT VERIFIED. Native domain tests are a separate check."), 11);
    auto* Right = Panel(true, 430);
    AddText(Right, TEXT("OPERATIONS"), 22);
    LocationPicker = AddPicker(Right, {}); LocationPicker->OnSelectionChanged.AddDynamic(this, &UMaiHUDWidget::LocationChanged);
    LocationText = AddText(Right, TEXT("Select a location"), 14);
    PagePicker = AddPicker(Right, {TEXT("Location & slots"), TEXT("Procurement & installation"), TEXT("Warehouse & orders"), TEXT("Save / load"), TEXT("Auction"), TEXT("Regions & power")});
    PagePicker->OnSelectionChanged.AddDynamic(this, &UMaiHUDWidget::PageChanged);
    Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(); Right->AddChild(Pages);
    const auto Page = [&]() { auto* Box = WidgetTree->ConstructWidget<UVerticalBox>(); Pages->AddChild(Box); return Box; };
    auto* LocationPage = Page(); AddButton(LocationPage, TEXT("Buy selected location"), TEXT("buy"));
    AddText(LocationPage, TEXT("Slots are separate from the electricity limit. The Garage cannot power nine Terra T1 chips."), 12);
    Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(); Grid->SetSlotPadding(FMargin(2)); LocationPage->AddChild(Grid);
    AddButton(LocationPage, TEXT("Open procurement for selected cell"), TEXT("procurepage"));
    auto* BuyPage = Page();
    AddText(BuyPage, TEXT("Chassis")); RackPicker = AddPicker(BuyPage, {TEXT("Basic rack"), TEXT("Cooled rack"), TEXT("Enterprise rack")});
    AddText(BuyPage, TEXT("Chip")); ChipPicker = AddPicker(BuyPage, {TEXT("Terra T1"), TEXT("Titan X9"), TEXT("Helios HC"), TEXT("Zenith Z1")});
    AddText(BuyPage, TEXT("Channel")); ChannelPicker = AddPicker(BuyPage, {TEXT("Official x1.6"), TEXT("Grey x1.1 / profile-dependent defect at installation")});
    AddText(BuyPage, TEXT("Quantity, 1-24")); QuantityInput = AddInput(BuyPage, TEXT("1")); QuoteText = AddText(BuyPage, TEXT(""));
    AddButton(BuyPage, TEXT("Order complete kit"), TEXT("kit")); AddButton(BuyPage, TEXT("Order chassis only"), TEXT("rackorder"));
    AddButton(BuyPage, TEXT("Order chip to warehouse"), TEXT("chiporder")); AddButton(BuyPage, TEXT("Order chip upgrade for this cell"), TEXT("upgradeorder"));
    AddText(BuyPage, TEXT("Installation uses delivered stock. It never pays the order again."), 12);
    AddButton(BuyPage, TEXT("Install chassis from warehouse"), TEXT("mountrack")); AddButton(BuyPage, TEXT("Install / upgrade chip from warehouse"), TEXT("mountchip"));
    auto* StockPage = Page(); AddText(StockPage, TEXT("WAREHOUSE"), 18); InventoryText = AddText(StockPage, TEXT("")); AddText(StockPage, TEXT("IN TRANSIT"), 18); OrdersText = AddText(StockPage, TEXT(""));
    auto* SavePage = Page(); AddText(SavePage, TEXT("Save slot: letters, numbers, underscore or hyphen")); SlotInput = AddInput(SavePage, TEXT("MakeYourAI_01"));
    AddButton(SavePage, TEXT("Save company"), TEXT("save")); AddButton(SavePage, TEXT("Load company"), TEXT("load"));
    AddButton(SavePage, TEXT("New company (press twice)"), TEXT("new"));
    AddText(SavePage, TEXT("Loading validates the whole snapshot before replacing state. Browser IndexedDB saves are not compatible with this schema."), 12);
    auto* AuctionPage = Page(); AuctionText = AddText(AuctionPage, TEXT(""));
    AddButton(AuctionPage, TEXT("Open configured auction / deliver to selected location"), TEXT("auctionopen"));
    AddText(AuctionPage, TEXT("Bid in whole dollars")); BidInput = AddInput(AuctionPage, TEXT("5000")); AddButton(AuctionPage, TEXT("Place bid in escrow"), TEXT("bid"));
    auto* RegionPage = Page(); RegionPicker = AddPicker(RegionPage, {TEXT("home"), TEXT("overseas"), TEXT("greenhaven")}); RegionText = AddText(RegionPage, TEXT(""));
    AddButton(RegionPage, TEXT("Unlock region"), TEXT("unlockregion")); AddButton(RegionPage, TEXT("Switch region"), TEXT("switchregion"));
    AddButton(RegionPage, TEXT("Buy company nuclear station"), TEXT("nuclear"));
    AddText(RegionPage, TEXT("Nuclear and Greenhaven require explicit BALANCE_TUNABLE configuration. No free power plant or invented income is enabled by default."), 12);
}
void UMaiHUDWidget::NativeConstruct() {
    Super::NativeConstruct();
    Company = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>() : nullptr;
    if (LocationPicker && Company && Company->Domain()) {
        LocationPicker->ClearOptions();
        for (const auto& L : Company->Domain()->Definitions().locations) LocationPicker->AddOption(UTF8_TO_TCHAR(L.id.c_str()));
        LocationPicker->SetSelectedOption(SelectedLocation);
    }
    RebuildGrid(); Refresh();
    if (GetWorld()) GetWorld()->GetTimerManager().SetTimer(RefreshTimer, this, &UMaiHUDWidget::Refresh, 0.2f, true);
}
void UMaiHUDWidget::NativeDestruct() {
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RefreshTimer);
    Company = nullptr; Super::NativeDestruct();
}
void UMaiHUDWidget::RebuildGrid() {
    if (!Grid || !Company || !Company->Domain()) return;
    Grid->ClearChildren(); CellLabels.Reset();
    const int I = Company->Domain()->Definitions().LocationIndex(TCHAR_TO_UTF8(*SelectedLocation)); if (I < 0) return;
    const auto& D = Company->Domain()->Definitions().locations[static_cast<std::size_t>(I)];
    for (int32 Row = 0; Row < D.rows; ++Row) for (int32 Col = 0; Col < D.cols; ++Col) {
        const int32 Cell = Row * D.cols + Col;
        auto* Button = WidgetTree->ConstructWidget<UMaiActionButton>(); Button->Configure(this, FString::Printf(TEXT("cell:%d"), Cell));
        Button->SetBackgroundColor(FLinearColor(0.13f, 0.13f, 0.15f, 1));
        auto* Label = AddText(nullptr, FString::Printf(TEXT("%d:%d"), Row + 1, Col + 1), 12); Button->SetContent(Label); CellLabels.Add(Label);
        Grid->AddChildToUniformGrid(Button, Row, Col);
    }
}
