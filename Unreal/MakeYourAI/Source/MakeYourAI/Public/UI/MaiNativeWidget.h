#pragma once
#include "CoreMinimal.h"
#include "Types/SlateEnums.h"
#include "Blueprint/UserWidget.h"
#include "Dom/JsonObject.h"
#include "MaiNativeWidget.generated.h"
class UCanvasPanel;
class UBorder;
class UTextBlock;
class UMaiRulesSubsystem;
class UMaiNativeWidget;
UCLASS()
class MAKEYOURAI_API UMaiNativeBinding : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UMaiNativeWidget> Owner;
    FString Id;
    UFUNCTION() void Click();
    UFUNCTION() void TextChanged(const FText& Text);
    UFUNCTION() void SelectionChanged(FString Selection,ESelectInfo::Type Type);
    UFUNCTION() void SliderChanged(float Value);
    UFUNCTION() UWidget* GenerateOption(FString Option);
};
UCLASS()
class MAKEYOURAI_API UMaiNativeWidget : public UUserWidget {
    GENERATED_BODY()
public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    void Dispatch(const FString& Id,const TSharedPtr<FJsonValue>& Value=nullptr);
    bool ValidateViewport(FString& Error) const;
    bool IsWalkingView() const;
    bool CanControlMap(bool CheckPointer) const;
    bool RevealContentNode(const FString& Id);
    bool ApplyingSnapshot=false;
    UFUNCTION(BlueprintPure,Category="MakeYourAI|UI") TArray<FString> ActionIds() const;
private:
    UPROPERTY(Transient) TObjectPtr<UMaiRulesSubsystem> Rules;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaiNativeBinding>> Bindings;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UWidget>> Widgets;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UTextBlock>> Labels;
    TMap<FString,TSharedPtr<FJsonObject>> Nodes;
    TMap<FString,TSharedPtr<SWidget>> SlateCharts;
    TSharedPtr<FJsonObject> Snapshot;
    FString Structure;
    TMap<FString,FVector> MapMarkerPositions;
    float RefreshClock=0;
    UWidget* BuildNode(const TSharedPtr<FJsonObject>& Node);
    void ApplyNode(const TSharedPtr<FJsonObject>& Node);
    void Refresh();
    void Rebuild(const TSharedPtr<FJsonObject>& View);
    void Arrange();
    void BuildMapMarkers();
    FString Signature(const TSharedPtr<FJsonObject>& Node) const;
};
