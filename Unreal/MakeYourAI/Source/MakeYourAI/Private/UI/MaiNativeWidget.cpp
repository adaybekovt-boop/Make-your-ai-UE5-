#include "UI/MaiNativeWidget.h"
#include "Rules/MaiRulesSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/NativeWidgetHost.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "World/MaiCityBatch.h"
#include "World/MaiPlayerController.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
namespace {
FString Str(const TSharedPtr<FJsonObject>& N,const TCHAR* Key,const FString& Default={}){FString S;return N&&N->TryGetStringField(Key,S)?S:Default;}
// Native vector equivalents of src/ui/Icon.tsx; no webview or icon-font dependency.
class SMaiIcon final:public SLeafWidget {
public:
    SLATE_BEGIN_ARGS(SMaiIcon){} SLATE_ARGUMENT(FString,Name) SLATE_END_ARGS()
    FString Name;
    void Construct(const FArguments& Args){Name=Args._Name;}
    virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(18,18);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool)const override {
        const auto Line=[&](std::initializer_list<FVector2D> Input){TArray<FVector2D> P;for(auto V:Input)P.Add(V*G.GetLocalSize()/24.f);FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,FLinearColor(.64,.74,.72)*Style.GetColorAndOpacityTint(),true,1.3f);};
        const auto Circle=[&](float X,float Y,float R){TArray<FVector2D>P;for(int I=0;I<=32;++I){float A=I*2*PI/32;P.Add(FVector2D(X+R*FMath::Cos(A),Y+R*FMath::Sin(A))*G.GetLocalSize()/24.f);}FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,FLinearColor(.64,.74,.72)*Style.GetColorAndOpacityTint(),true,1.3f);};
        if(Name==TEXT("pause")){Line({{8,5},{8,19}});Line({{16,5},{16,19}});}
        else if(Name==TEXT("play"))Line({{7,4},{20,12},{7,20},{7,4}});
        else if(Name==TEXT("save")){Line({{3,3},{17,3},{21,7},{21,21},{3,21},{3,3}});Line({{7,3},{7,9},{16,9},{16,3}});Line({{7,21},{7,14},{17,14},{17,21}});}
        else if(Name==TEXT("settings")){Line({{4,7},{20,7}});Line({{4,17},{20,17}});Circle(9,7,2.5);Circle(15,17,2.5);}
        else if(Name==TEXT("clock")){Circle(12,12,9);Line({{12,6},{12,12},{16,14}});}
        else if(Name==TEXT("help")){Circle(12,12,9);Line({{9,9},{10,7},{14,7},{15,10},{12,13},{12,14}});Circle(12,17,.5);}
        else if(Name==TEXT("model")){Line({{7,7},{17,7},{17,17},{7,17},{7,7}});Line({{12,2},{12,7}});Line({{12,17},{12,22}});Line({{2,12},{7,12}});Line({{17,12},{22,12}});Line({{5,5},{8,8}});Line({{16,16},{19,19}});Line({{19,5},{16,8}});Line({{8,16},{5,19}});}
        else if(Name==TEXT("flask")){Line({{10,3},{14,3}});Line({{11,3},{11,9},{5,19},{7,21},{17,21},{19,19},{13,9},{13,3}});Line({{8,14},{16,14}});}
        else if(Name==TEXT("wallet")){Line({{20,8},{20,3},{6,3},{3,5},{3,18},{6,21},{20,21},{20,9},{6,9},{3,7}});Line({{20,12},{14,12},{14,17},{20,17}});}
        else if(Name==TEXT("server")){for(float Y:{3.f,14.f}){Line({{4,Y},{20,Y},{20,Y+7},{4,Y+7},{4,Y}});Line({{12,Y+3.5f},{17,Y+3.5f}});Circle(8,Y+3.5f,.6);}}
        else if(Name==TEXT("building")){Line({{3,21},{21,21},{21,10},{12,3},{3,10},{3,21}});Line({{8,21},{8,13},{16,13},{16,21}});Line({{8,16},{16,16}});}
        else if(Name==TEXT("campus")){Line({{2,8},{12,3},{22,8},{2,8}});for(float X:{5.f,12.f,19.f})Line({{X,10},{X,19}});Line({{2,21},{22,21}});}
        return Layer+1;
    }
};
TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& N,const TCHAR* Key){const TSharedPtr<FJsonObject>* V=nullptr;return N&&N->TryGetObjectField(Key,V)?*V:nullptr;}
FLinearColor Color(const FString& Role){if(Role==TEXT("danger"))return FLinearColor(.88f,.43f,.36f);if(Role==TEXT("positive")||Role==TEXT("primary")||Role==TEXT("selected"))return FLinearColor(.46f,.78f,.65f);return FLinearColor(.88f,.90f,.87f);}
class SMaiTrainingRing final:public SLeafWidget {
public:
    SLATE_BEGIN_ARGS(SMaiTrainingRing) {} SLATE_END_ARGS()
    void Construct(const FArguments&){}
    float Progress=-1.f;
    virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(180,180);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool)const override {
        const FVector2D Center=G.GetLocalSize()*.5f;
        const float Radius=FMath::Max(0.f,FMath::Min(Center.X,Center.Y)-18.f);
        const auto Arc=[&](float Fraction,const FLinearColor& Tint,int32 Z){
            TArray<FVector2D> Points;const int32 Segments=FMath::Max(1,FMath::CeilToInt(96*Fraction));
            for(int32 I=0;I<=Segments;++I){const float A=-PI*.5f+2*PI*Fraction*I/Segments;Points.Add(Center+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius);}
            FSlateDrawElement::MakeLines(Out,Z,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Tint*Style.GetColorAndOpacityTint(),true,5.f);
        };
        Arc(1.f,FLinearColor(.035f,.075f,.078f),Layer);
        if(Progress>0.f)Arc(FMath::Clamp(Progress,0.f,1.f),Color(TEXT("positive")),Layer+1);
        return Layer+2;
    }
};
class SMaiHistory final:public SLeafWidget {
public:
    SLATE_BEGIN_ARGS(SMaiHistory) {} SLATE_END_ARGS()
    void Construct(const FArguments&){}
    TArray<float> Samples;
    virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(440,112);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override {
        if(Samples.Num()<2)return Layer;
        float Lo=Samples[0],Hi=Lo;for(float P:Samples){Lo=FMath::Min(Lo,P);Hi=FMath::Max(Hi,P);}const float Range=FMath::Max(1.f,Hi-Lo);
        TArray<FVector2D> Points;const FVector2D Size=G.GetLocalSize();
        for(int32 I=0;I<Samples.Num();++I)Points.Add(FVector2D(8+(Size.X-16)*I/(Samples.Num()-1),Size.Y-8-(Size.Y-16)*(Samples[I]-Lo)/Range));
        FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color(TEXT("positive")),true,2.f);return Layer+1;
    }
};
}
void UMaiNativeBinding::Click(){if(Owner)Owner->Dispatch(Id);}
void UMaiNativeBinding::TextChanged(const FText& T){if(Owner)Owner->Dispatch(Id,MakeShared<FJsonValueString>(T.ToString()));}
void UMaiNativeBinding::SelectionChanged(FString S,ESelectInfo::Type){if(Owner)Owner->Dispatch(Id,MakeShared<FJsonValueString>(S));}
void UMaiNativeBinding::SliderChanged(float V){if(Owner)Owner->Dispatch(Id,MakeShared<FJsonValueNumber>(V));}
UWidget* UMaiNativeBinding::GenerateOption(FString Option){
    auto* Label=NewObject<UTextBlock>(Owner);Label->SetText(FText::FromString(Option));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),12));
    Label->SetColorAndOpacity(FLinearColor(.88f,.90f,.87f));return Label;
}
TSharedRef<SWidget> UMaiNativeWidget::RebuildWidget(){
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this,TEXT("NativeWidgetTree"));
    Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);WidgetTree->RootWidget=Canvas;
    return Super::RebuildWidget();
}
void UMaiNativeWidget::NativeConstruct(){Super::NativeConstruct();Rules=GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>();SetVisibility(ESlateVisibility::SelfHitTestInvisible);Refresh();}
bool UMaiNativeWidget::IsWalkingView()const{return Snapshot&&Str(Obj(Snapshot,TEXT("content")),TEXT("id"))==TEXT("walk-prompt")&&!Obj(Snapshot,TEXT("modal"));}
bool UMaiNativeWidget::CanControlMap(bool CheckPointer)const{
    if(!Snapshot||Str(Snapshot,TEXT("mode"))!=TEXT("world")||Str(Snapshot,TEXT("page"))!=TEXT("map")||Obj(Snapshot,TEXT("modal"))||IsWalkingView())return false;
    if(CheckPointer){
        if(!FSlateApplication::IsInitialized())return false;
        const FVector2D PointerPosition=FSlateApplication::Get().GetCursorPos();
        for(const TCHAR* Id:{TEXT("__toolbar_frame"),TEXT("__content_frame")})if(auto* Panel=Widgets.FindRef(Id).Get())
            if(Panel->IsVisible()&&Panel->GetCachedGeometry().IsUnderLocation(PointerPosition))return false;
    }
    return true;
}
bool UMaiNativeWidget::RevealContentNode(const FString& Id){
    auto* Scroll=Cast<UScrollBox>(Widgets.FindRef(TEXT("__content")));auto* Target=Widgets.FindRef(Id).Get();
    if(!Scroll||!Target)return false;
    Scroll->ScrollWidgetIntoView(Target,false,EDescendantScrollDestination::TopOrLeft);return true;
}
FReply UMaiNativeWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event){
    if(IsWalkingView() && Event.GetKey()==EKeys::Tab){
        if(auto* PC=Cast<AMaiPlayerController>(GetOwningPlayer())){PC->ToggleWalkCursor();return FReply::Handled();}
    }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
void UMaiNativeWidget::NativeTick(const FGeometry& G,float Delta){Super::NativeTick(G,Delta);RefreshClock+=Delta;if(RefreshClock>=.2f){RefreshClock=0;Refresh();}Arrange();}
FString UMaiNativeWidget::Signature(const TSharedPtr<FJsonObject>& N)const {
    if(!N)return TEXT("-");FString S=Str(N,TEXT("kind"))+TEXT(":")+Str(N,TEXT("id"))+Str(N,TEXT("icon"));
    const TArray<TSharedPtr<FJsonValue>>* A=nullptr;
    if(N->TryGetArrayField(TEXT("children"),A))for(const auto& V:*A)S+=TEXT("[")+Signature(V->AsObject())+TEXT("]");
    if(N->TryGetArrayField(TEXT("options"),A))for(const auto& V:*A)S+=Str(V->AsObject(),TEXT("label"))+Str(V->AsObject(),TEXT("value"));
    return S;
}
void UMaiNativeWidget::Refresh(){if(!Rules)return;auto View=Rules->ViewModel();if(!View.IsValid())return;Snapshot=View;
    const FString S=Str(View,TEXT("mode"))+Signature(Obj(View,TEXT("toolbar")))+Signature(Obj(View,TEXT("content")))+Signature(Obj(View,TEXT("modal")));
    TGuardValue<bool> Guard(ApplyingSnapshot,true);if(S!=Structure){Structure=S;Rebuild(View);}else{ApplyNode(Obj(View,TEXT("toolbar")));ApplyNode(Obj(View,TEXT("content")));ApplyNode(Obj(View,TEXT("modal")));}
    if(auto* Notice=Labels.FindRef(TEXT("__notice")).Get())Notice->SetText(FText::FromString(Str(View,TEXT("notice"))));
    ForceLayoutPrepass();
}
void UMaiNativeWidget::Dispatch(const FString& Id,const TSharedPtr<FJsonValue>& Value){
    if(ApplyingSnapshot||!Rules)return;const auto* Found=Nodes.Find(Id);if(!Found||!Found->IsValid())return;const auto N=*Found;
    bool Enabled=true;N->TryGetBoolField(TEXT("enabled"),Enabled);if(!Enabled)return;
    TArray<TSharedPtr<FJsonValue>> Args;const TArray<TSharedPtr<FJsonValue>>* Old=nullptr;if(N->TryGetArrayField(TEXT("args"),Old))Args=*Old;
    if(Value){auto Submitted=Value;const FString Kind=Str(N,TEXT("kind"));
        if(Kind==TEXT("select")){const TArray<TSharedPtr<FJsonValue>>* Options=nullptr;if(N->TryGetArrayField(TEXT("options"),Options))for(const auto& O:*Options)if(Str(O->AsObject(),TEXT("label"))==Value->AsString()){Submitted=MakeShared<FJsonValueString>(Str(O->AsObject(),TEXT("value")));break;}}
        if(Kind==TEXT("slider")){double Step=1;N->TryGetNumberField(TEXT("step"),Step);Submitted=MakeShared<FJsonValueNumber>(FMath::RoundToDouble(Value->AsNumber()/Step)*Step);}
        Args.Add(Submitted);
    }
    Rules->Dispatch(Str(N,TEXT("action")),Args);Refresh();
}
UWidget* UMaiNativeWidget::BuildNode(const TSharedPtr<FJsonObject>& N){
    if(!N)return nullptr;const FString Id=Str(N,TEXT("id")),Kind=Str(N,TEXT("kind"));Nodes.Add(Id,N);UWidget* W=nullptr;
    auto Text=[&](){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetAutoWrapText(!Id.StartsWith(TEXT("map-location-")));T->SetFont(FCoreStyle::GetDefaultFontStyle(Kind==TEXT("heading")?TEXT("Bold"):TEXT("Regular"),Id==TEXT("brand")?28:Kind==TEXT("heading")?21:12));T->SetColorAndOpacity(Color(Str(N,TEXT("role"))));T->SetText(FText::FromString(Str(N,TEXT("label"))));Labels.Add(Id,T);return T;};
    UMaiNativeBinding* Binding=nullptr;
    if(N->HasField(TEXT("action"))){Binding=NewObject<UMaiNativeBinding>(this);Binding->Owner=this;Binding->Id=Id;Bindings.Add(Binding);}
    if(Kind==TEXT("text")||Kind==TEXT("heading"))W=Text();
    else if(Kind==TEXT("button")){auto* B=WidgetTree->ConstructWidget<UButton>();
        const FString Icon=Str(N,TEXT("icon"));
        if(!Icon.IsEmpty()){
            auto* H=WidgetTree->ConstructWidget<UNativeWidgetHost>();H->SetContent(SNew(SMaiIcon).Name(Icon));
            auto* Contents=WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* IconSlot=Contents->AddChildToHorizontalBox(H);IconSlot->SetVerticalAlignment(VAlign_Center);IconSlot->SetPadding(FMargin(0,0,Id.StartsWith(TEXT("map-location-"))?0:8,0));
            Contents->AddChildToHorizontalBox(Text())->SetVerticalAlignment(VAlign_Center);B->AddChild(Contents);
        }else B->AddChild(Text());
        B->OnClicked.AddDynamic(Binding,&UMaiNativeBinding::Click);auto Style=B->GetStyle();Style.Normal=FSlateRoundedBoxBrush(FLinearColor::White,8.f);Style.Hovered=FSlateRoundedBoxBrush(FLinearColor(1.2f,1.2f,1.2f),8.f);Style.Pressed=FSlateRoundedBoxBrush(FLinearColor(.75f,.75f,.75f),8.f);
        const FString Role=Str(N,TEXT("role"));Style.NormalPadding=Role==TEXT("metric")?FMargin(0):Role==TEXT("brand")?FMargin(6,10):FMargin(10,9);Style.PressedPadding=Style.NormalPadding;B->SetStyle(Style);W=B;}
    else if(Kind==TEXT("spacer"))W=WidgetTree->ConstructWidget<USpacer>();
    else if(Kind==TEXT("grid")){
        auto* Grid=WidgetTree->ConstructWidget<UUniformGridPanel>();Grid->SetSlotPadding(FMargin(6));Grid->SetMinDesiredSlotWidth(100);Grid->SetMinDesiredSlotHeight(72);
        double Columns=1;N->TryGetNumberField(TEXT("columns"),Columns);const int32 Count=FMath::Max(1,int32(Columns));
        const TArray<TSharedPtr<FJsonValue>>* Children=nullptr;if(N->TryGetArrayField(TEXT("children"),Children))for(int32 I=0;I<Children->Num();++I)if(auto* Child=BuildNode((*Children)[I]->AsObject())){auto* CellSlot=Grid->AddChildToUniformGrid(Child,I/Count,I%Count);CellSlot->SetHorizontalAlignment(HAlign_Fill);CellSlot->SetVerticalAlignment(VAlign_Fill);}W=Grid;
    }
    else if(Kind==TEXT("input")){auto* E=WidgetTree->ConstructWidget<UEditableTextBox>();auto InputStyle=E->GetWidgetStyle();InputStyle.BackgroundImageNormal=FSlateRoundedBoxBrush(FLinearColor(.022f,.043f,.05f),8.f);InputStyle.BackgroundImageHovered=InputStyle.BackgroundImageNormal;InputStyle.BackgroundImageFocused=FSlateRoundedBoxBrush(FLinearColor(.035f,.06f,.07f),8.f);InputStyle.ForegroundColor=FLinearColor(.88f,.92f,.91f);InputStyle.TextStyle.Font=FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),15);InputStyle.Padding=FMargin(12,10);E->SetWidgetStyle(InputStyle);E->SetHintText(FText::FromString(Str(N,TEXT("label"))));E->SetMinDesiredWidth(280);E->OnTextChanged.AddDynamic(Binding,&UMaiNativeBinding::TextChanged);W=E;}
    else if(Kind==TEXT("select")){
        auto* C=WidgetTree->ConstructWidget<UComboBoxString>();auto Style=C->GetWidgetStyle();
        Style.ComboButtonStyle.ButtonStyle.Normal=FSlateRoundedBoxBrush(FLinearColor(.026f,.05f,.06f),6.f);
        Style.ComboButtonStyle.ButtonStyle.Hovered=FSlateRoundedBoxBrush(FLinearColor(.05f,.085f,.09f),6.f);
        Style.ComboButtonStyle.ButtonStyle.Pressed=Style.ComboButtonStyle.ButtonStyle.Hovered;
        Style.ComboButtonStyle.ButtonStyle.NormalPadding=FMargin(12,10);
        Style.ComboButtonStyle.ButtonStyle.PressedPadding=FMargin(12,10);
        Style.ComboButtonStyle.MenuBorderBrush=FSlateRoundedBoxBrush(FLinearColor(.015f,.03f,.035f),8.f);C->SetWidgetStyle(Style);
        C->OnGenerateWidgetEvent.BindDynamic(Binding,&UMaiNativeBinding::GenerateOption);
        const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("options"),A))for(const auto& O:*A)C->AddOption(Str(O->AsObject(),TEXT("label")));C->OnSelectionChanged.AddDynamic(Binding,&UMaiNativeBinding::SelectionChanged);W=C;
    }
    else if(Kind==TEXT("slider")){auto* S=WidgetTree->ConstructWidget<USlider>();S->OnValueChanged.AddDynamic(Binding,&UMaiNativeBinding::SliderChanged);W=S;}
    else if(Kind==TEXT("progress")){W=WidgetTree->ConstructWidget<UProgressBar>();}
    else if(Kind==TEXT("ring")){
        auto* Overlay=WidgetTree->ConstructWidget<UOverlay>();
        auto* Host=WidgetTree->ConstructWidget<UNativeWidgetHost>();auto Ring=SNew(SMaiTrainingRing);Host->SetContent(Ring);SlateCharts.Add(Id,Ring);
        auto* RingSlot=Overlay->AddChildToOverlay(Host);RingSlot->SetHorizontalAlignment(HAlign_Fill);RingSlot->SetVerticalAlignment(VAlign_Fill);
        auto* Core=WidgetTree->ConstructWidget<UVerticalBox>();
        auto* Value=WidgetTree->ConstructWidget<UTextBlock>();Value->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"),32));Value->SetColorAndOpacity(Color(TEXT("positive")));Value->SetJustification(ETextJustify::Center);Labels.Add(Id+TEXT("__value"),Value);Core->AddChild(Value);
        auto* Caption=Text();Caption->SetJustification(ETextJustify::Center);Core->AddChild(Caption);
        auto* CoreSlot=Overlay->AddChildToOverlay(Core);CoreSlot->SetHorizontalAlignment(HAlign_Center);CoreSlot->SetVerticalAlignment(VAlign_Center);
        W=Overlay;
    }
    else if(Kind==TEXT("chart")){auto* H=WidgetTree->ConstructWidget<UNativeWidgetHost>();auto Chart=SNew(SMaiHistory);const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("samples"),A))for(const auto& V:*A)Chart->Samples.Add(float(V->AsNumber()));H->SetContent(Chart);SlateCharts.Add(Id,Chart);W=H;}
    else {
        UPanelWidget* P=nullptr;if(Kind==TEXT("bar")||Kind==TEXT("columns"))P=WidgetTree->ConstructWidget<UHorizontalBox>();else if(Kind==TEXT("row")){auto* Wrap=WidgetTree->ConstructWidget<UWrapBox>();Wrap->SetInnerSlotPadding(FVector2D(8,8));P=Wrap;}else P=WidgetTree->ConstructWidget<UVerticalBox>();
        const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("children"),A))for(const auto& Child:*A){if(auto* Item=BuildNode(Child->AsObject())){auto* ChildSlot=P->AddChild(Item);if(auto* V=Cast<UVerticalBoxSlot>(ChildSlot))V->SetPadding(FMargin(0,0,0,Id.EndsWith(TEXT("-metric"))?3:10));if(auto* H=Cast<UHorizontalBoxSlot>(ChildSlot)){H->SetVerticalAlignment(VAlign_Center);H->SetPadding(FMargin(4,0));if(Str(Child->AsObject(),TEXT("kind"))==TEXT("spacer"))H->SetSize(FSlateChildSize(ESlateSizeRule::Fill));}}}
        if(Kind==TEXT("columns"))for(auto* ChildSlot:P->GetSlots())if(auto* H=Cast<UHorizontalBoxSlot>(ChildSlot)){H->SetSize(FSlateChildSize(ESlateSizeRule::Fill));H->SetVerticalAlignment(VAlign_Top);H->SetPadding(FMargin(0,0,12,0));}
        if(Kind==TEXT("card")){auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.014f,.029f,.034f,.98f),12.f));B->SetPadding(FMargin(16));B->AddChild(P);W=B;}else W=P;
    }
    if(Kind==TEXT("card")&&Id.StartsWith(TEXT("competence-"))){
        auto* Border=CastChecked<UBorder>(W);Border->SetPadding(FMargin(12,8));
        if(auto* Stack=Cast<UVerticalBox>(Border->GetContent()))for(auto* ChildSlot:Stack->GetSlots())if(auto* V=Cast<UVerticalBoxSlot>(ChildSlot))V->SetPadding(FMargin(0,0,0,3));
    }
    if(!W)W=Text();Widgets.Add(Id,W);ApplyNode(N);
    if(auto* T=Labels.FindRef(Id).Get()){
        if(Id==TEXT("brand-button"))T->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"),18));
        else if(Id==TEXT("capital-label")||Id==TEXT("profit-label")){T->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),9));T->SetColorAndOpacity(FLinearColor(.4,.5,.5));}
        if(Id==TEXT("cash")||Id==TEXT("profit"))T->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"),13));
    }
    if(Kind==TEXT("slider")||Kind==TEXT("progress")||Kind==TEXT("select")||Kind==TEXT("chart")) {
        auto* Group=WidgetTree->ConstructWidget<UVerticalBox>();Group->AddChild(Text());Group->AddChild(W);return Group;
    }
    return W;
}
void UMaiNativeWidget::ApplyNode(const TSharedPtr<FJsonObject>& N){
    if(!N)return;const FString Id=Str(N,TEXT("id"));Nodes.Add(Id,N);UWidget* W=Widgets.FindRef(Id);if(!W)return;
    bool Enabled=true;N->TryGetBoolField(TEXT("enabled"),Enabled);W->SetIsEnabled(Enabled);W->SetToolTipText(FText::FromString(Str(N,TEXT("tooltip"))));
    if(auto* T=Labels.FindRef(Id).Get())T->SetText(FText::FromString(Str(N,TEXT("label"))));
    if(auto* B=Cast<UButton>(W)){const FString R=Str(N,TEXT("role"));if(auto* Label=Labels.FindRef(Id).Get())Label->SetColorAndOpacity(R==TEXT("primary")||R==TEXT("selected")?FLinearColor(.008f,.019f,.024f):Color(R));B->SetBackgroundColor(R==TEXT("primary")||R==TEXT("selected")?FLinearColor(.29f,.63f,.49f):FLinearColor(.022f,.043f,.05f));}
    if(auto* B=Cast<UButton>(W)){const auto Role=Str(N,TEXT("role"));if(Role==TEXT("metric")||Role==TEXT("brand"))B->SetBackgroundColor(FLinearColor(0,0,0,0));}
    if(auto* E=Cast<UEditableTextBox>(W)){if(!E->HasKeyboardFocus())E->SetText(FText::FromString(Str(N,TEXT("value"))));}
    if(auto* C=Cast<UComboBoxString>(W)){const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("options"),A))for(const auto& O:*A)if(Str(O->AsObject(),TEXT("value"))==Str(N,TEXT("value")))C->SetSelectedOption(Str(O->AsObject(),TEXT("label")));}
    if(auto* S=Cast<USlider>(W)){S->SetMinValue(float(N->GetNumberField(TEXT("min"))));S->SetMaxValue(float(N->GetNumberField(TEXT("max"))));S->SetStepSize(float(N->GetNumberField(TEXT("step"))));S->SetValue(float(N->GetNumberField(TEXT("value"))));}
    if(auto* P=Cast<UProgressBar>(W))P->SetPercent(float(N->GetNumberField(TEXT("value"))));
    if(SlateCharts.Contains(Id)&&Str(N,TEXT("kind"))==TEXT("ring")){
        auto Ring=StaticCastSharedPtr<SMaiTrainingRing>(SlateCharts.FindRef(Id));double Value=-1;N->TryGetNumberField(TEXT("value"),Value);
        Ring->Progress=float(Value);Ring->Invalidate(EInvalidateWidgetReason::Paint);
        if(auto* Label=Labels.FindRef(Id+TEXT("__value")).Get())Label->SetText(FText::FromString(Value<0?TEXT("—"):FString::Printf(TEXT("%d%%"),FMath::RoundToInt(FMath::Clamp(Value,0.,1.)*100))));
    }
    else if(SlateCharts.Contains(Id)&&Str(N,TEXT("kind"))==TEXT("chart")){auto Chart=StaticCastSharedPtr<SMaiHistory>(SlateCharts.FindRef(Id));if(Chart.IsValid()){Chart->Samples.Reset();const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("samples"),A))for(const auto& V:*A)Chart->Samples.Add(float(V->AsNumber()));Chart->Invalidate(EInvalidateWidgetReason::Paint);}}
    const TArray<TSharedPtr<FJsonValue>>* Children=nullptr;if(N->TryGetArrayField(TEXT("children"),Children))for(const auto& C:*Children)ApplyNode(C->AsObject());
}
void UMaiNativeWidget::Rebuild(const TSharedPtr<FJsonObject>& View){
    // Structural updates only; numeric ticks preserve text focus, scroll and keyboard navigation.
    TMap<FString,float> Scrolls;for(const TCHAR* Name:{TEXT("__content"),TEXT("__modal")})if(auto* S=Cast<UScrollBox>(Widgets.FindRef(Name)))Scrolls.Add(Name,S->GetScrollOffset());
    Bindings.Reset();Widgets.Reset();Labels.Reset();Nodes.Reset();SlateCharts.Reset();if(!Canvas)return;Canvas->ClearChildren();
    if(Str(View,TEXT("mode"))==TEXT("full")){auto* Backdrop=WidgetTree->ConstructWidget<UBorder>();Backdrop->SetBrushColor(FLinearColor(.005f,.011f,.014f));Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);auto* BackdropSlot=Canvas->AddChildToCanvas(Backdrop);BackdropSlot->SetAnchors(FAnchors(0,0,1,1));BackdropSlot->SetOffsets(FMargin(0));}
    auto Pane=[&](const TCHAR* Id,const TSharedPtr<FJsonObject>& Node,bool Scroll){if(!Node)return;auto* Border=WidgetTree->ConstructWidget<UBorder>();Border->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.008f,.019f,.024f,.98f),16.f));Border->SetPadding(FMargin(16));UWidget* Child=BuildNode(Node);if(Scroll){auto* S=WidgetTree->ConstructWidget<UScrollBox>();S->SetAnimateWheelScrolling(false);S->AddChild(Child);Border->AddChild(S);Widgets.Add(Id,S);if(const float* Offset=Scrolls.Find(Id))S->SetScrollOffset(*Offset);}else{Border->AddChild(Child);Widgets.Add(Id,Border);}Canvas->AddChild(Border);Widgets.Add(FString(Id)+TEXT("_frame"),Border);};
    MapMarkerPositions.Reset();if(Str(View,TEXT("mode"))==TEXT("world")&&Str(Obj(View,TEXT("content")),TEXT("id"))!=TEXT("walk-prompt"))BuildMapMarkers();
    Pane(TEXT("__toolbar"),Obj(View,TEXT("toolbar")),false);Pane(TEXT("__content"),Obj(View,TEXT("content")),true);
    auto* Dot=WidgetTree->ConstructWidget<UBorder>();Dot->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White,3.f));Dot->SetPadding(FMargin(0));
    auto* DotSlot=Canvas->AddChildToCanvas(Dot);DotSlot->SetAnchors(FAnchors(.5f,.5f));DotSlot->SetAlignment(FVector2D(.5f,.5f));DotSlot->SetSize(FVector2D(5,5));DotSlot->SetZOrder(20);Widgets.Add(TEXT("__crosshair"),Dot);
    auto* WalkHUD=WidgetTree->ConstructWidget<UBorder>();WalkHUD->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.008f,.019f,.024f,.85f),12.f));WalkHUD->SetPadding(FMargin(16,12));
    auto* WalkText=WidgetTree->ConstructWidget<UTextBlock>();WalkText->SetAutoWrapText(true);WalkText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),13));WalkText->SetColorAndOpacity(FLinearColor(.8f,.92f,.9f));WalkHUD->AddChild(WalkText);Canvas->AddChild(WalkHUD);Widgets.Add(TEXT("__walk_hud"),WalkHUD);Labels.Add(TEXT("__walk_hud"),WalkText);
    if(Obj(View,TEXT("modal"))){auto* Shade=WidgetTree->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(0,0,0,.65f));auto* ChildSlot=Canvas->AddChildToCanvas(Shade);ChildSlot->SetAnchors(FAnchors(0,0,1,1));ChildSlot->SetOffsets(FMargin(0));Pane(TEXT("__modal"),Obj(View,TEXT("modal")),true);}
    auto* Notice=WidgetTree->ConstructWidget<UTextBlock>();Notice->SetAutoWrapText(true);Notice->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),14));Notice->SetColorAndOpacity(Color(TEXT("positive")));Labels.Add(TEXT("__notice"),Notice);Canvas->AddChild(Notice);Arrange();
}
void UMaiNativeWidget::Arrange(){if(!Canvas||!Snapshot)return;const float Scale=UWidgetLayoutLibrary::GetViewportScale(this);FVector2D Size=UWidgetLayoutLibrary::GetViewportSize(this)/FMath::Max(.01f,Scale);if(Size.X<1||Size.Y<1)return;
    const bool Walking=IsWalkingView()&&GetOwningPlayer()&&!GetOwningPlayer()->bShowMouseCursor;
    for(const TCHAR* Name:{TEXT("__toolbar_frame"),TEXT("__content_frame")})if(auto* W=Widgets.FindRef(Name).Get())W->SetVisibility(Walking?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
    float Top=12;
    if(auto* W=Widgets.FindRef(TEXT("__toolbar_frame")).Get()){auto* S=Cast<UCanvasPanelSlot>(W->Slot);S->SetPosition(FVector2D(0,0));S->SetSize(FVector2D(Size.X,FMath::Max(64.f,float(W->GetDesiredSize().Y))));Top=S->GetSize().Y+16;}
    const FString Mode=Str(Snapshot,TEXT("mode"));
    auto Position=[&](const TCHAR* Name,bool Modal){auto* W=Widgets.FindRef(FString(Name)+TEXT("_frame")).Get();if(!W)return;auto* S=Cast<UCanvasPanelSlot>(W->Slot);
        const bool Small=!Modal&&Mode==TEXT("world");const bool Menu=Str(Obj(Snapshot,TEXT("content")),TEXT("id"))==TEXT("menu");const float Width=FMath::Min(Size.X-24,Small?272.f:Modal?860.f:Menu?460.f:Mode==TEXT("full")?600.f:1080.f);
        const bool Center=Modal||Mode==TEXT("full");const float Available=Size.Y-(Center?64:Top+52);const float Height=Small?FMath::Min(float(W->GetDesiredSize().Y)+8,Available):Center?FMath::Clamp(float(W->GetDesiredSize().Y)+24.f,360.f,FMath::Max(360.f,Available)):Available;S->SetPosition(FVector2D(Small?12:(Size.X-Width)/2,Center?(Size.Y-Height)/2:Top));S->SetSize(FVector2D(Width,FMath::Max(80.f,Height)));};
    Position(TEXT("__content"),false);Position(TEXT("__modal"),true);
    if(auto* Dot=Widgets.FindRef(TEXT("__crosshair")).Get())Dot->SetVisibility(Walking?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(auto* HUD=Widgets.FindRef(TEXT("__walk_hud")).Get()){
        HUD->SetVisibility(Walking?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        auto* S=Cast<UCanvasPanelSlot>(HUD->Slot);S->SetPosition(FVector2D(20,Size.Y-110));S->SetSize(FVector2D(FMath::Min(600.,Size.X-40),90));
        if(auto* T=Labels.FindRef(TEXT("__walk_hud")).Get())if(auto* Hint=Labels.FindRef(TEXT("walk-hint")).Get())T->SetText(Hint->GetText());
    }
    TArray<FSlateRect> PlacedMarkers;TArray<FString> MarkerIds;MapMarkerPositions.GetKeys(MarkerIds);
    const FString SelectedMarker=TEXT("map-location-")+Str(Snapshot,TEXT("selectedLocation"));
    MarkerIds.Sort([&](const FString& A,const FString& B){if(A==SelectedMarker)return B!=SelectedMarker;if(B==SelectedMarker)return false;return A<B;});
    for(const auto& MarkerId:MarkerIds){auto* W=Widgets.FindRef(MarkerId).Get();if(!W)continue;FVector2D Point;
        const bool Selected=MarkerId==SelectedMarker,Expanded=Selected||W->IsHovered()||W->HasKeyboardFocus();
        W->SetToolTipText(Expanded?FText::GetEmpty():FText::FromString(Str(Nodes.FindRef(MarkerId),TEXT("tooltip"))));
        const float Width=Expanded?FMath::Clamp(50.f+Str(Nodes.FindRef(MarkerId),TEXT("label")).Len()*8.f,120.f,246.f):42.f,Height=38.f;
        if(auto* Label=Labels.FindRef(MarkerId).Get()){Label->SetVisibility(Expanded?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);Label->SetColorAndOpacity(FLinearColor(.88f,.94f,.92f));}
        if(auto* Button=Cast<UButton>(W))Button->SetBackgroundColor(Selected?FLinearColor(.055f,.22f,.18f):FLinearColor(.012f,.035f,.043f));
        const bool Visible=UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(),MapMarkerPositions[MarkerId],Point,false)&&Point.X>0&&Point.X<Size.X&&Point.Y>Top&&Point.Y<Size.Y-60;
        W->SetVisibility(Visible?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(Visible){FVector2D MarkerPosition(FMath::Clamp(Point.X-21,12.,Size.X-Width-12),Point.Y);
            for(int32 Attempt=0;Attempt<8;++Attempt){bool Overlap=false;for(const FSlateRect& Other:PlacedMarkers)if(MarkerPosition.X<Other.Right+6&&MarkerPosition.X+Width+6>Other.Left&&MarkerPosition.Y<Other.Bottom+6&&MarkerPosition.Y+Height+6>Other.Top){Overlap=true;break;}if(!Overlap)break;MarkerPosition.Y+=44;}
            if(MarkerPosition.Y+Height>Size.Y-48){W->SetVisibility(ESlateVisibility::Collapsed);continue;}
            PlacedMarkers.Add(FSlateRect(MarkerPosition.X,MarkerPosition.Y,MarkerPosition.X+Width,MarkerPosition.Y+Height));auto* S=Cast<UCanvasPanelSlot>(W->Slot);S->SetPosition(MarkerPosition);S->SetSize(FVector2D(Width,Height));}}
    if(auto* T=Labels.FindRef(TEXT("__notice")).Get()){T->SetVisibility(Walking?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);auto* S=Cast<UCanvasPanelSlot>(T->Slot);S->SetPosition(FVector2D(20,Size.Y-44));S->SetSize(FVector2D(Size.X-40,38));}
}
void UMaiNativeWidget::BuildMapMarkers(){
    const TMap<FString,FString> Names={{TEXT("garage"),TEXT("Гараж")},{TEXT("workshop"),TEXT("Мастерская")},{TEXT("technopark"),TEXT("Технопарк")},{TEXT("server-hall"),TEXT("Серверный цех")},{TEXT("campus"),TEXT("Кампус")},{TEXT("dc-north"),TEXT("Северный дата-центр")},{TEXT("dc-south"),TEXT("Южный дата-центр")}};
    for(TActorIterator<AMaiCityBatch> It(GetWorld());It;++It){const FString* Name=Names.Find(It->LocationId);if(!Name||!It->Instances)continue;
        const FString Id=TEXT("map-location-")+It->LocationId;if(MapMarkerPositions.Contains(Id))continue;
        auto N=MakeShared<FJsonObject>();N->SetStringField(TEXT("kind"),TEXT("button"));N->SetStringField(TEXT("id"),Id);N->SetStringField(TEXT("label"),*Name);N->SetStringField(TEXT("action"),TEXT("ui:location"));N->SetArrayField(TEXT("args"),{MakeShared<FJsonValueString>(It->LocationId)});
        N->SetStringField(TEXT("tooltip"),*Name);N->SetStringField(TEXT("label"),TEXT("  ")+*Name);
        N->SetStringField(TEXT("icon"),It->LocationId==TEXT("campus")?TEXT("campus"):It->LocationId.StartsWith(TEXT("dc-"))||It->LocationId==TEXT("server-hall")?TEXT("server"):TEXT("building"));
        if(auto* W=BuildNode(N)){Canvas->AddChild(W);MapMarkerPositions.Add(Id,It->Instances->Bounds.Origin+FVector(0,0,It->Instances->Bounds.BoxExtent.Z+100));}
    }
}
TArray<FString> UMaiNativeWidget::ActionIds()const{TArray<FString> Result;for(const auto& Pair:Nodes)if(Pair.Value->HasField(TEXT("action")))Result.Add(Pair.Key);Result.Sort();return Result;}

bool UMaiNativeWidget::ValidateViewport(FString& Error)const{
    const auto View=UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(GetOwningPlayer());const auto Size=View.GetLocalSize();
    if(Size.X<100||Size.Y<100){Error=TEXT("Viewport is not arranged");return false;}
    for(const auto& Pair:Widgets){if(!Pair.Key.StartsWith(TEXT("nav-"))&&Pair.Key!=TEXT("new-game")&&Pair.Key!=TEXT("quit")&&Pair.Key!=TEXT("settings"))continue;
        auto* W=Pair.Value.Get();if(!W||!W->IsVisible())continue;const auto G=W->GetCachedGeometry();const auto A=View.AbsoluteToLocal(G.LocalToAbsolute(FVector2D::ZeroVector));const auto B=View.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
        if(B.X-A.X<1||B.Y-A.Y<1||A.X < -1||A.Y < -1||B.X>Size.X+1||B.Y>Size.Y+1){Error=TEXT("Control clipped or unarranged: ")+Pair.Key;return false;}}
    return true;
}
