#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MaiFlowWidget.generated.h"
class UMaiFlowWidget;
class UMaiCompanySubsystem;
class UPanelWidget;
class UCanvasPanel;
class UVerticalBox;
class UTextBlock;
class UEditableTextBox;
class UProgressBar;
UCLASS()
class MAKEYOURAI_API UMaiCampaignButton : public UButton {
    GENERATED_BODY()
public:
    void Configure(UMaiFlowWidget* Owner,const FString& InCommand);
private:
    UPROPERTY(Transient) TObjectPtr<UMaiFlowWidget> Screen;
    FString Command;
    UFUNCTION() void Execute();
};
// Programmatic UMG screens. No designer .uasset is required to instantiate them.
UCLASS()
class MAKEYOURAI_API UMaiFlowWidget : public UUserWidget {
    GENERATED_BODY()
public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    void Command(const FString& Action);
    void Refresh();
private:
    UPROPERTY(Transient) TObjectPtr<UMaiCompanySubsystem> Company;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Content;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Summary;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LoadingText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> JobText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> CompanyName;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> Progress;
    FString Signature,Message;
    int64 SelectedBatch=0;
    float RefreshElapsed=0;
    UTextBlock* Text(UPanelWidget* Parent,const FString& Value,int32 Size=15);
    UMaiCampaignButton* Button(UPanelWidget* Parent,const FString& Label,const FString& Action,bool Enabled=true);
    UPanelWidget* Row(UPanelWidget* Parent);
    void Build();
    void BuildTraining();
    void BuildEnding();
};
