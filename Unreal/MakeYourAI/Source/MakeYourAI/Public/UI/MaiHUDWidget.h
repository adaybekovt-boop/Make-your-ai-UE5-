#pragma once
#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Gameplay/MaiViewTypes.h"
#include "MaiHUDWidget.generated.h"
class UPanelWidget;
class UTextBlock;
class UEditableTextBox;
class UUniformGridPanel;
class UWidgetSwitcher;
class UMaiCompanySubsystem;
class UMaiHUDWidget;

UCLASS()
class MAKEYOURAI_API UMaiActionButton : public UButton {
    GENERATED_BODY()
public:
    void Configure(UMaiHUDWidget* Owner, const FString& InCommand);
private:
    UPROPERTY(Transient) TObjectPtr<UMaiHUDWidget> Screen;
    UPROPERTY(Transient) FString Command;
    UFUNCTION() void Execute();
};

UCLASS()
class MAKEYOURAI_API UMaiHUDWidget : public UUserWidget {
    GENERATED_BODY()
public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    void HandleCommand(const FString& Command);
    void SelectLocation(const FString& Id, int32 Cell = -1);
protected:
    UPROPERTY(Transient) TObjectPtr<UMaiCompanySubsystem> Company;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CompanyText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LocationText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> InventoryText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> OrdersText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> NotificationText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> QuoteText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> AuctionText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RegionText;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> LocationPicker;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> PagePicker;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> RackPicker;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> ChipPicker;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> ChannelPicker;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> RegionPicker;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> QuantityInput;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> SlotInput;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> BidInput;
    UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> Grid;
    UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> CellLabels;
    FString SelectedLocation = TEXT("garage");
    int32 SelectedCell = 0;
    FText ActionMessage;
    FTimerHandle RefreshTimer;
    bool bNewCompanyArmed = false;
    void BuildLayout();
    void Refresh();
    void RebuildGrid();
    UTextBlock* AddText(UPanelWidget* Parent, const FString& Text, int32 FontSize = 14);
    UMaiActionButton* AddButton(UPanelWidget* Parent, const FString& Label, const FString& Command);
    UComboBoxString* AddPicker(UPanelWidget* Parent, const TArray<FString>& Options);
    UEditableTextBox* AddInput(UPanelWidget* Parent, const FString& Default);
    void ShowResult(const FMaiActionResult& Result);
    UFUNCTION() void LocationChanged(FString Selection, ESelectInfo::Type Type);
    UFUNCTION() void PageChanged(FString Selection, ESelectInfo::Type Type);
};
