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
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "World/MaiCityBatch.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
namespace {
FString Str(const TSharedPtr<FJsonObject>& N,const TCHAR* Key,const FString& Default={}){FString S;return N&&N->TryGetStringField(Key,S)?S:Default;}
TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& N,const TCHAR* Key){const TSharedPtr<FJsonObject>* V=nullptr;return N&&N->TryGetObjectField(Key,V)?*V:nullptr;}
FLinearColor Color(const FString& Role){if(Role==TEXT("danger"))return FLinearColor(.88f,.43f,.36f);if(Role==TEXT("positive")||Role==TEXT("primary")||Role==TEXT("selected"))return FLinearColor(.46f,.78f,.65f);return FLinearColor(.88f,.90f,.87f);}
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
TSharedRef<SWidget> UMaiNativeWidget::RebuildWidget(){
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this,TEXT("NativeWidgetTree"));
    Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);WidgetTree->RootWidget=Canvas;
    return Super::RebuildWidget();
}
void UMaiNativeWidget::NativeConstruct(){Super::NativeConstruct();Rules=GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>();SetVisibility(ESlateVisibility::SelfHitTestInvisible);Refresh();}
void UMaiNativeWidget::NativeTick(const FGeometry& G,float Delta){Super::NativeTick(G,Delta);RefreshClock+=Delta;if(RefreshClock>=.2f){RefreshClock=0;Refresh();}Arrange();}
FString UMaiNativeWidget::Signature(const TSharedPtr<FJsonObject>& N)const {
    if(!N)return TEXT("-");FString S=Str(N,TEXT("kind"))+TEXT(":")+Str(N,TEXT("id"));
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
    else if(Kind==TEXT("button")){auto* B=WidgetTree->ConstructWidget<UButton>();auto* T=Text();B->AddChild(T);B->OnClicked.AddDynamic(Binding,&UMaiNativeBinding::Click);auto Style=B->GetStyle();Style.Normal=FSlateRoundedBoxBrush(FLinearColor::White,8.f);Style.Hovered=FSlateRoundedBoxBrush(FLinearColor(1.2f,1.2f,1.2f),8.f);Style.Pressed=FSlateRoundedBoxBrush(FLinearColor(.75f,.75f,.75f),8.f);Style.NormalPadding=FMargin(14,10);Style.PressedPadding=FMargin(14,10);B->SetStyle(Style);W=B;}
    else if(Kind==TEXT("input")){auto* E=WidgetTree->ConstructWidget<UEditableTextBox>();auto InputStyle=E->GetWidgetStyle();InputStyle.BackgroundImageNormal=FSlateRoundedBoxBrush(FLinearColor(.022f,.043f,.05f),8.f);InputStyle.BackgroundImageHovered=InputStyle.BackgroundImageNormal;InputStyle.BackgroundImageFocused=FSlateRoundedBoxBrush(FLinearColor(.035f,.06f,.07f),8.f);InputStyle.ForegroundColor=FLinearColor(.88f,.92f,.91f);InputStyle.TextStyle.Font=FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),15);InputStyle.Padding=FMargin(12,10);E->SetWidgetStyle(InputStyle);E->SetHintText(FText::FromString(Str(N,TEXT("label"))));E->SetMinDesiredWidth(280);E->OnTextChanged.AddDynamic(Binding,&UMaiNativeBinding::TextChanged);W=E;}
    else if(Kind==TEXT("select")){auto* C=WidgetTree->ConstructWidget<UComboBoxString>();const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("options"),A))for(const auto& O:*A)C->AddOption(Str(O->AsObject(),TEXT("label")));C->OnSelectionChanged.AddDynamic(Binding,&UMaiNativeBinding::SelectionChanged);W=C;}
    else if(Kind==TEXT("slider")){auto* S=WidgetTree->ConstructWidget<USlider>();S->OnValueChanged.AddDynamic(Binding,&UMaiNativeBinding::SliderChanged);W=S;}
    else if(Kind==TEXT("progress")){W=WidgetTree->ConstructWidget<UProgressBar>();}
    else if(Kind==TEXT("chart")){auto* H=WidgetTree->ConstructWidget<UNativeWidgetHost>();auto Chart=SNew(SMaiHistory);const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("samples"),A))for(const auto& V:*A)Chart->Samples.Add(float(V->AsNumber()));H->SetContent(Chart);SlateCharts.Add(Id,Chart);W=H;}
    else {
        UPanelWidget* P=nullptr;if(Kind==TEXT("row")){auto* Wrap=WidgetTree->ConstructWidget<UWrapBox>();Wrap->SetInnerSlotPadding(FVector2D(8,8));P=Wrap;}else P=WidgetTree->ConstructWidget<UVerticalBox>();
        const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("children"),A))for(const auto& Child:*A){if(auto* Item=BuildNode(Child->AsObject())){auto* ChildSlot=P->AddChild(Item);if(auto* V=Cast<UVerticalBoxSlot>(ChildSlot))V->SetPadding(FMargin(0,0,0,10));}}
        if(Kind==TEXT("card")){auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrushColor(FLinearColor(.014f,.029f,.034f,.98f));B->SetPadding(FMargin(16));B->AddChild(P);W=B;}else W=P;
    }
    if(!W)W=Text();Widgets.Add(Id,W);ApplyNode(N);
    if(Kind==TEXT("slider")||Kind==TEXT("progress")||Kind==TEXT("select")||Kind==TEXT("chart")) {
        auto* Group=WidgetTree->ConstructWidget<UVerticalBox>();Group->AddChild(Text());Group->AddChild(W);return Group;
    }
    return W;
}
void UMaiNativeWidget::ApplyNode(const TSharedPtr<FJsonObject>& N){
    if(!N)return;const FString Id=Str(N,TEXT("id"));Nodes.Add(Id,N);UWidget* W=Widgets.FindRef(Id);if(!W)return;
    bool Enabled=true;N->TryGetBoolField(TEXT("enabled"),Enabled);W->SetIsEnabled(Enabled);W->SetToolTipText(FText::FromString(Str(N,TEXT("label"))));
    if(auto* T=Labels.FindRef(Id).Get())T->SetText(FText::FromString(Str(N,TEXT("label"))));
    if(auto* B=Cast<UButton>(W)){const FString R=Str(N,TEXT("role"));if(auto* Label=Labels.FindRef(Id).Get())Label->SetColorAndOpacity(R==TEXT("primary")||R==TEXT("selected")?FLinearColor(.008f,.019f,.024f):Color(R));B->SetBackgroundColor(R==TEXT("primary")||R==TEXT("selected")?FLinearColor(.29f,.63f,.49f):FLinearColor(.022f,.043f,.05f));}
    if(auto* E=Cast<UEditableTextBox>(W)){if(!E->HasKeyboardFocus())E->SetText(FText::FromString(Str(N,TEXT("value"))));}
    if(auto* C=Cast<UComboBoxString>(W)){const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("options"),A))for(const auto& O:*A)if(Str(O->AsObject(),TEXT("value"))==Str(N,TEXT("value")))C->SetSelectedOption(Str(O->AsObject(),TEXT("label")));}
    if(auto* S=Cast<USlider>(W)){S->SetMinValue(float(N->GetNumberField(TEXT("min"))));S->SetMaxValue(float(N->GetNumberField(TEXT("max"))));S->SetStepSize(float(N->GetNumberField(TEXT("step"))));S->SetValue(float(N->GetNumberField(TEXT("value"))));}
    if(auto* P=Cast<UProgressBar>(W))P->SetPercent(float(N->GetNumberField(TEXT("value"))));
    if(SlateCharts.Contains(Id)){auto Chart=StaticCastSharedPtr<SMaiHistory>(SlateCharts.FindRef(Id));if(Chart.IsValid()){Chart->Samples.Reset();const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(N->TryGetArrayField(TEXT("samples"),A))for(const auto& V:*A)Chart->Samples.Add(float(V->AsNumber()));Chart->Invalidate(EInvalidateWidgetReason::Paint);}}
    const TArray<TSharedPtr<FJsonValue>>* Children=nullptr;if(N->TryGetArrayField(TEXT("children"),Children))for(const auto& C:*Children)ApplyNode(C->AsObject());
}
void UMaiNativeWidget::Rebuild(const TSharedPtr<FJsonObject>& View){
    // Structural updates only; numeric ticks preserve text focus, scroll and keyboard navigation.
    TMap<FString,float> Scrolls;for(const TCHAR* Name:{TEXT("__content"),TEXT("__modal")})if(auto* S=Cast<UScrollBox>(Widgets.FindRef(Name)))Scrolls.Add(Name,S->GetScrollOffset());
    Bindings.Reset();Widgets.Reset();Labels.Reset();Nodes.Reset();SlateCharts.Reset();if(!Canvas)return;Canvas->ClearChildren();
    if(Str(View,TEXT("mode"))==TEXT("full")){auto* Backdrop=WidgetTree->ConstructWidget<UBorder>();Backdrop->SetBrushColor(FLinearColor(.005f,.011f,.014f));Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);auto* BackdropSlot=Canvas->AddChildToCanvas(Backdrop);BackdropSlot->SetAnchors(FAnchors(0,0,1,1));BackdropSlot->SetOffsets(FMargin(0));}
    auto Pane=[&](const TCHAR* Id,const TSharedPtr<FJsonObject>& Node,bool Scroll){if(!Node)return;auto* Border=WidgetTree->ConstructWidget<UBorder>();Border->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.008f,.019f,.024f,.98f),16.f));Border->SetPadding(FMargin(16));UWidget* Child=BuildNode(Node);if(Scroll){auto* S=WidgetTree->ConstructWidget<UScrollBox>();S->SetAnimateWheelScrolling(false);S->AddChild(Child);Border->AddChild(S);Widgets.Add(Id,S);if(const float* Offset=Scrolls.Find(Id))S->SetScrollOffset(*Offset);}else{Border->AddChild(Child);Widgets.Add(Id,Border);}Canvas->AddChild(Border);Widgets.Add(FString(Id)+TEXT("_frame"),Border);};
    MapMarkerPositions.Reset();if(Str(View,TEXT("mode"))==TEXT("world"))BuildMapMarkers();
    Pane(TEXT("__toolbar"),Obj(View,TEXT("toolbar")),false);Pane(TEXT("__content"),Obj(View,TEXT("content")),true);
    if(Obj(View,TEXT("modal"))){auto* Shade=WidgetTree->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(0,0,0,.65f));auto* ChildSlot=Canvas->AddChildToCanvas(Shade);ChildSlot->SetAnchors(FAnchors(0,0,1,1));ChildSlot->SetOffsets(FMargin(0));Pane(TEXT("__modal"),Obj(View,TEXT("modal")),true);}
    auto* Notice=WidgetTree->ConstructWidget<UTextBlock>();Notice->SetAutoWrapText(true);Notice->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),14));Notice->SetColorAndOpacity(Color(TEXT("positive")));Labels.Add(TEXT("__notice"),Notice);Canvas->AddChild(Notice);Arrange();
}
void UMaiNativeWidget::Arrange(){if(!Canvas||!Snapshot)return;const float Scale=UWidgetLayoutLibrary::GetViewportScale(this);FVector2D Size=UWidgetLayoutLibrary::GetViewportSize(this)/FMath::Max(.01f,Scale);if(Size.X<1||Size.Y<1)return;
    float Top=12;
    if(auto* W=Widgets.FindRef(TEXT("__toolbar_frame")).Get()){auto* S=Cast<UCanvasPanelSlot>(W->Slot);S->SetPosition(FVector2D(12,12));S->SetSize(FVector2D(Size.X-24,FMath::Max(64.f,float(W->GetDesiredSize().Y))));Top=12+S->GetSize().Y+12;}
    const FString Mode=Str(Snapshot,TEXT("mode"));
    auto Position=[&](const TCHAR* Name,bool Modal){auto* W=Widgets.FindRef(FString(Name)+TEXT("_frame")).Get();if(!W)return;auto* S=Cast<UCanvasPanelSlot>(W->Slot);
        const bool Small=!Modal&&Mode==TEXT("world");const bool Menu=Str(Obj(Snapshot,TEXT("content")),TEXT("id"))==TEXT("menu");const float Width=FMath::Min(Size.X-24,Small?272.f:Modal?860.f:Menu?460.f:Mode==TEXT("full")?600.f:1080.f);
        const bool Center=Modal||Mode==TEXT("full");const float Available=Size.Y-(Center?64:Top+52);const float Height=Small?FMath::Min(430.f,Available):Center?FMath::Clamp(float(W->GetDesiredSize().Y)+24.f,360.f,FMath::Max(360.f,Available)):Available;S->SetPosition(FVector2D(Small?12:(Size.X-Width)/2,Center?(Size.Y-Height)/2:Top));S->SetSize(FVector2D(Width,FMath::Max(80.f,Height)));};
    Position(TEXT("__content"),false);Position(TEXT("__modal"),true);
    TArray<FVector2D> PlacedMarkers;
    for(const auto& Marker:MapMarkerPositions){auto* W=Widgets.FindRef(Marker.Key).Get();if(!W)continue;FVector2D Point;
        const bool Visible=UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(),Marker.Value,Point,false)&&Point.X>0&&Point.X<Size.X&&Point.Y>Top&&Point.Y<Size.Y-60;
        W->SetVisibility(Visible?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(Visible){FVector2D MarkerPosition(FMath::Clamp(Point.X-110,12.,Size.X-232),Point.Y);
            for(int32 Attempt=0;Attempt<8;++Attempt){bool Overlap=false;for(const FVector2D& Other:PlacedMarkers)if(FMath::Abs(MarkerPosition.X-Other.X)<228&&FMath::Abs(MarkerPosition.Y-Other.Y)<52){Overlap=true;break;}if(!Overlap)break;MarkerPosition.Y+=52;}
            if(MarkerPosition.Y+46>Size.Y-48){W->SetVisibility(ESlateVisibility::Collapsed);continue;}
            PlacedMarkers.Add(MarkerPosition);auto* S=Cast<UCanvasPanelSlot>(W->Slot);S->SetPosition(MarkerPosition);S->SetSize(FVector2D(220,46));}}
    if(auto* T=Labels.FindRef(TEXT("__notice")).Get()){auto* S=Cast<UCanvasPanelSlot>(T->Slot);S->SetPosition(FVector2D(20,Size.Y-44));S->SetSize(FVector2D(Size.X-40,38));}
}
void UMaiNativeWidget::BuildMapMarkers(){
    const TMap<FString,FString> Names={{TEXT("garage"),TEXT("Гараж")},{TEXT("workshop"),TEXT("Мастерская")},{TEXT("technopark"),TEXT("Технопарк")},{TEXT("server-hall"),TEXT("Серверный цех")},{TEXT("campus"),TEXT("Кампус")},{TEXT("dc-north"),TEXT("Северный дата-центр")},{TEXT("dc-south"),TEXT("Южный дата-центр")}};
    for(TActorIterator<AMaiCityBatch> It(GetWorld());It;++It){const FString* Name=Names.Find(It->LocationId);if(!Name||!It->Instances)continue;
        const FString Id=TEXT("map-location-")+It->LocationId;if(MapMarkerPositions.Contains(Id))continue;
        auto N=MakeShared<FJsonObject>();N->SetStringField(TEXT("kind"),TEXT("button"));N->SetStringField(TEXT("id"),Id);N->SetStringField(TEXT("label"),*Name);N->SetStringField(TEXT("action"),TEXT("ui:location"));N->SetArrayField(TEXT("args"),{MakeShared<FJsonValueString>(It->LocationId)});
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
