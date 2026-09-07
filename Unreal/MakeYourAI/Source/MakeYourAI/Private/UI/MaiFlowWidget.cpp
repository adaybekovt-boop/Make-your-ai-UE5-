#include "UI/MaiFlowWidget.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Campaign/MaiLoadingSubsystem.h"
#include "World/MaiPlayerController.h"
#include "Persistence/MaiSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Misc/LexFromString.h"
namespace {
FString S(const std::string& V){return UTF8_TO_TCHAR(V.c_str());}
FString M(mai::Money V){return S(mai::FormatMoney(V));}
constexpr const TCHAR* Slot=TEXT("MakeYourAI_01");
}
void UMaiCampaignButton::Configure(UMaiFlowWidget* Owner,const FString& InCommand){Screen=Owner;Command=InCommand;OnClicked.AddUniqueDynamic(this,&UMaiCampaignButton::Execute);}
void UMaiCampaignButton::Execute(){if(Screen) Screen->Command(Command);}
TSharedRef<SWidget> UMaiFlowWidget::RebuildWidget(){
    if(!WidgetTree) WidgetTree=NewObject<UWidgetTree>(this,TEXT("CampaignTree"));
    if(!WidgetTree->RootWidget){Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);WidgetTree->RootWidget=Canvas;}
    return Super::RebuildWidget();
}
void UMaiFlowWidget::NativeConstruct(){Super::NativeConstruct();Company=GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();Refresh();}
void UMaiFlowWidget::NativeTick(const FGeometry& G,float Delta){Super::NativeTick(G,Delta);RefreshElapsed+=Delta;if(RefreshElapsed>=.2f){RefreshElapsed=0;Refresh();}}
UTextBlock* UMaiFlowWidget::Text(UPanelWidget* Parent,const FString& Value,int32 Size){
    auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));T->SetAutoWrapText(true);auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);T->SetColorAndOpacity(FSlateColor(FLinearColor(.88f,.9f,.93f,1)));if(Parent) Parent->AddChild(T);return T;
}
UMaiCampaignButton* UMaiFlowWidget::Button(UPanelWidget* Parent,const FString& Label,const FString& Action,bool Enabled){
    auto* B=WidgetTree->ConstructWidget<UMaiCampaignButton>();B->Configure(this,Action);B->SetIsEnabled(Enabled);B->SetBackgroundColor(FLinearColor(.13f,.15f,.18f,1));
    auto* Pad=WidgetTree->ConstructWidget<UBorder>();Pad->SetPadding(FMargin(12,8));Pad->SetBrushColor(FLinearColor::Transparent);Pad->SetContent(Text(nullptr,Label));B->SetContent(Pad);if(Parent){Parent->AddChild(B);if(auto* H=Cast<UHorizontalBoxSlot>(B->Slot)) H->SetSize(FSlateChildSize(ESlateSizeRule::Fill));}return B;
}
UPanelWidget* UMaiFlowWidget::Row(UPanelWidget* Parent){auto* R=WidgetTree->ConstructWidget<UHorizontalBox>();Parent->AddChild(R);return R;}
void UMaiFlowWidget::Build(){
    if(!Canvas || !Company || !Company->CampaignDomain()) return;
    const auto& G=*Company->CampaignDomain();const auto& V=G.View();
    Canvas->ClearChildren();Summary=nullptr;LoadingText=nullptr;JobText=nullptr;CompanyName=nullptr;Progress=nullptr;Feedback=nullptr;
    const bool Compact=V.screen==mai::Screen::CityMap || V.screen==mai::Screen::Gameplay;
    auto* Background=WidgetTree->ConstructWidget<UBorder>();Background->SetBrushColor(FLinearColor(.018f,.022f,.028f,Compact?.97f:1.f));Background->SetPadding(FMargin(22));
    auto* Position=Canvas->AddChildToCanvas(Background);
    if(Compact){Position->SetAnchors(FAnchors(.22f,1,.78f,1));Position->SetOffsets(FMargin(0,-150,0,140));}
    else{Position->SetAnchors(FAnchors(0,0,1,1));Position->SetOffsets(FMargin(0));}
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Background->SetContent(Scroll);Content=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Content);
    Text(Content,Compact?TEXT("CAMPAIGN"):TEXT("NEURON / MAKE YOUR AI"),Compact?14:30);
    Summary=Text(Content,TEXT(""),14);Feedback=Text(Content,Message,14);
    auto* Saves=GetGameInstance()->GetSubsystem<UMaiSaveSubsystem>();
    if(V.screen==mai::Screen::Loading){
        Text(Content,TEXT("Loading"),24);LoadingText=Text(Content,TEXT(""));Progress=WidgetTree->ConstructWidget<UProgressBar>();Content->AddChild(Progress);
        if(V.loading.phase==mai::LoadPhase::Failed) Button(Content,TEXT("Retry required load"),TEXT("retry"));
    }else if(V.screen==mai::Screen::MainMenu){
        Text(Content,TEXT("Build a company. Review your data. Live with the decisions."),20);
        Button(Content,TEXT("New game"),TEXT("new"));Button(Content,TEXT("Load saved company"),TEXT("load"),Saves && Saves->HasSave(Slot));
        if(!V.difficulty.empty() && V.prologueStep==3 && !G.Core().View().ended) Button(Content,TEXT("Resume current company"),TEXT("resume"));
    }else if(V.screen==mai::Screen::NewGame){
        Text(Content,TEXT("Create a new company"),24);Text(Content,TEXT("Existing save slots are retained. Difficulty is locked once chosen."));Button(Content,TEXT("Choose difficulty"),TEXT("difficulty"));
    }else if(V.screen==mai::Screen::Difficulty){
        Text(Content,TEXT("Choose your starting conditions"),24);
        for(const auto& D:G.Rules().difficulties){
            Text(Content,FString::Printf(TEXT("%s / capital %s / factory risk %.1f%% / hiring %.0f%% / legal pressure %.0f%% / grace %lld h"),*S(D.id),*M(mai::Dollars(12000)*D.capitalBps/10000),.08*D.defectBps/100.,D.hiringBps/100.,D.legalBps/100.,static_cast<long long>(D.insolvencyGrace/mai::Hour)),16);
            Button(Content,S(D.id),TEXT("difficulty:")+S(D.id));
        }
        Text(Content,TEXT("Standard retains the existing equipment prices and delivery rules. New difficulty/review/endgame tuning is provisional."),12);
    }else if(V.screen==mai::Screen::Prologue){
        Text(Content,TEXT("Your first company"),24);
        if(V.prologueStep==0){CompanyName=WidgetTree->ConstructWidget<UEditableTextBox>();CompanyName->SetText(FText::FromString(TEXT("Neuron Labs")));Content->AddChild(CompanyName);Button(Content,TEXT("Register the company name"),TEXT("prologue-name"));}
        else if(V.prologueStep==1){
            Text(Content,TEXT("Before buying anything, inspect three real costs:"),18);
            Text(Content,TEXT("Garage $1,500; official basic rack + Terra T1 $4,800. A licensed 10-unit data sample costs $1,400. Review staff also charge money. Ownership incurs running costs."));
            Button(Content,TEXT("Inspect budget and accept these costs"),TEXT("prologue-budget"));
        }else{Text(Content,TEXT("First task: buy Garage, order and install one server, then check a dataset at the work desk. No free starter equipment is added."));Button(Content,TEXT("Accept task and load city"),TEXT("prologue-task"));}
    }else if(Compact){
        auto* R=Row(Content);Button(R,TEXT("Data / training"),TEXT("training"));Button(R,TEXT("Operations"),TEXT("operations"));Button(R,TEXT("Save"),TEXT("save"));Button(R,TEXT("Menu"),TEXT("menu"));
        if(V.screen==mai::Screen::Gameplay){Text(Content,TEXT("WASD: walk. E or a nearby click: interact. Manual review begins at the desk."),12);Button(Content,TEXT("Return to city map"),TEXT("city"));}
    }else if(V.screen==mai::Screen::Training) BuildTraining();
    else if(V.screen==mai::Screen::Ending) BuildEnding();
}
void UMaiFlowWidget::BuildTraining(){
    const auto& G=*Company->CampaignDomain();const auto& V=G.View();auto* PC=Cast<AMaiPlayerController>(GetOwningPlayer());
    Text(Content,TEXT("DATA / REVIEW / TRAINING"),24);
    if(V.ending.offerOnly){Text(Content,S(V.ending.title),22);Text(Content,S(V.ending.line),18);Text(Content,TEXT("Offered deal: ")+M(V.ending.deal),20);Text(Content,S(G.Rules().buyer.fallback),13);Text(Content,S(G.Rules().buyer.biography),13);}
    auto* Toolbar=Row(Content);Button(Toolbar,TEXT("Back"),TEXT("back"));Button(Toolbar,G.Core().View().paused?TEXT("Resume time"):TEXT("Pause time"),TEXT("pause"));Button(Toolbar,TEXT("Save"),TEXT("save"));Button(Toolbar,TEXT("Reload"),TEXT("load"));
    Text(Content,TEXT("Acquire a batch"),20);
    for(const auto& O:G.Rules().offers){auto* R=Row(Content);Text(R,FString::Printf(TEXT("%s / %d units / %s / legal %.0f%%  "),*S(O.name),O.volume,*M(O.cost),O.quality.legalBps/100.));Button(R,TEXT("Purchase"),TEXT("buy-data:")+S(O.id),G.Core().View().cash>=O.cost);}
    Text(Content,TEXT("Dataset inventory"),20);
    for(const auto& B:V.inventory.batches){
        auto* R=Row(Content);Text(R,FString::Printf(TEXT("#%lld %s / %s / quality %.1f%% / noise %.1f%% / legal %.1f%%  "),static_cast<long long>(B.id),*S(B.offerId),*S(mai::DatasetStatusName(B.status)),B.quality.scoreBps/100.,B.quality.noiseBps/100.,B.quality.legalBps/100.));
        Button(R,B.id==SelectedBatch?TEXT("Selected"):TEXT("Select"),FString::Printf(TEXT("select:%lld"),static_cast<long long>(B.id)));
    }
    const auto* B=G.Batch(SelectedBatch);
    if(B){
        Text(Content,FString::Printf(TEXT("Selected #%lld / %d units / paid %s / accepted %.1f%% / review %.3f game h"),static_cast<long long>(B->id),B->volume,*M(B->cost),B->quality.acceptedBps/100.,static_cast<double>(B->reviewTime)/mai::Hour),18);
        auto* R=Row(Content);const bool Unreviewed=B->status==mai::DatasetStatus::Unreviewed;
        Button(R,TEXT("Manual / at Garage desk"),TEXT("review:manual"),Unreviewed && PC && PC->AtReviewDesk());
        Button(R,FString::Printf(TEXT("Human / batch fee %s"),*M(V.specialists.empty()?G.Rules().humanBatch:V.specialists.front().batchFee)),TEXT("review:human"),Unreviewed && !V.specialists.empty());
        Button(R,TEXT("AI / consumes server compute"),TEXT("review:ai"),Unreviewed && V.ai.created);
        Button(R,TEXT("Train verified batch"),TEXT("train"),B->status==mai::DatasetStatus::Verified && G.AvailableTrainingCompute()>0);
    }
    auto* Staff=Row(Content);const auto* D=G.Difficulty();
    Button(Staff,FString::Printf(TEXT("Hire reviewer / %s"),*M(G.Rules().humanHire*(D?D->hiringBps:10000)/10000)),TEXT("hire"),V.specialists.size()<4);
    Button(Staff,FString::Printf(TEXT("Create AI reviewer / %s"),*M(G.Rules().aiSetup)),TEXT("ai-create"),!V.ai.created);
    Button(Staff,FString::Printf(TEXT("Upgrade reviewer / %s"),*M(G.Rules().aiUpgrade)),TEXT("ai-upgrade"),V.ai.created && V.ai.level<5);
    Button(Staff,V.overwork?TEXT("Normal staff hours"):TEXT("Request overtime"),TEXT("overwork"));
    Text(Content,FString::Printf(TEXT("Review specialists: %d / AI level %d / AI accuracy %.1f%% / systematic-bias risk %.1f%%"),static_cast<int32>(V.specialists.size()),V.ai.created?V.ai.level:0,V.ai.accuracyBps/100.,V.ai.biasBps/100.));
    for(const auto& W:V.specialists) Text(Content,FString::Printf(TEXT("Reviewer #%lld: %d units/h, accuracy %.1f%%, fatigue %.1f%%"),static_cast<long long>(W.id),W.volumePerHour,W.accuracyBps/100.,W.fatigueBps/100.));
    Text(Content,TEXT("Review queue and decisions"),20);
    for(const auto& R:V.reviews){
        Text(Content,FString::Printf(TEXT("Review #%lld / batch #%lld / method %d / %s"),static_cast<long long>(R.id),static_cast<long long>(R.batchId),static_cast<int32>(R.method),R.phase==mai::ReviewPhase::Queued?TEXT("queued"):R.phase==mai::ReviewPhase::Active?TEXT("active"):TEXT("completed")));
        if(R.method==mai::ReviewMethod::Manual && R.phase!=mai::ReviewPhase::Complete && R.decisions.size()<R.items.size()){
            const auto& I=R.items[R.decisions.size()];Text(Content,FString::Printf(TEXT("Stage %d / %d: %s"),static_cast<int32>(R.decisions.size()+1),static_cast<int32>(R.items.size()),*S(I.prompt)),20);
            if(I.type==mai::DataType::Image) Text(Content,TEXT("Image-review branch: captioned comparison cards. No photographic file is supplied in this scaffold."),12);
            auto* Choices=Row(Content);Button(Choices,S(I.left),FString::Printf(TEXT("choice:%lld:0"),static_cast<long long>(R.id)));Button(Choices,S(I.right),FString::Printf(TEXT("choice:%lld:1"),static_cast<long long>(R.id)));
        }else if(R.phase==mai::ReviewPhase::Complete){
            const auto* Batch=G.Batch(R.batchId);if(Batch) Text(Content,FString::Printf(TEXT("Accepted %.1f%% / detected noise %.1f%% / quality %.1f%% / %.3f game h / reputation %+d / training %s"),Batch->quality.acceptedBps/100.,Batch->quality.noiseBps/100.,R.resultQualityBps/100.,static_cast<double>(R.elapsed)/mai::Hour,R.reward,Batch->status==mai::DatasetStatus::Rejected?TEXT("blocked"):TEXT("quality weighted")));
        }
    }
    Text(Content,TEXT("Compute and current training"),20);JobText=Text(Content,TEXT(""));
    auto* Jobs=Row(Content);Button(Jobs,TEXT("Pause training job"),TEXT("job-pause"));Button(Jobs,TEXT("Resume training job"),TEXT("job-resume"));
    auto* Decisions=Row(Content);Button(Decisions,TEXT("Evaluate ending"),TEXT("evaluate"));Button(Decisions,TEXT("Choose open publication"),TEXT("open-model"));Button(Decisions,TEXT("Accept acquisition offer"),TEXT("sale"),V.ending.offerOnly);Button(Decisions,TEXT("Decline sale / stay independent"),TEXT("decline"));
    auto* Risks=Row(Content);Button(Risks,FString::Printf(TEXT("Remediate legal exposure / %s"),*M(G.Rules().remediationCost)),TEXT("remediate"));Button(Risks,TEXT("Ignore regulator warning"),TEXT("ignore"),V.warnings>V.ignoredWarnings);Button(Risks,TEXT("Competitor-backed loan $1,000"),TEXT("borrow"));Button(Risks,TEXT("Repay $1,000"),TEXT("repay"),V.debt>=mai::Dollars(1000));
    Text(Content,TEXT("Decision journal (most recent 12; full history is saved)"),18);
    for(size_t I=V.decisions.size()>12?V.decisions.size()-12:0;I<V.decisions.size();++I) Text(Content,S(V.decisions[I].action+": "+V.decisions[I].detail),13);
}
void UMaiFlowWidget::BuildEnding(){
    const auto& G=*Company->CampaignDomain();const auto& E=G.View().ending;const auto& V=E.metrics;
    Text(Content,S(E.title),28);Text(Content,S(E.line),22);
    if(E.kind==mai::EndingKind::Acquisition){
        const auto* A=Company->CampaignDefinitions();auto* Portrait=A?A->ElonMaxPortrait.Get():nullptr;
        if(Portrait){auto* Image=WidgetTree->ConstructWidget<UImage>();Image->SetBrushFromTexture(Portrait,true);Content->AddChild(Image);}
        else Text(Content,S(G.Rules().buyer.fallback),14);
        Text(Content,S(G.Rules().buyer.biography),14);Text(Content,TEXT("Deal: ")+M(E.deal),22);
    }
    Text(Content,FString::Printf(TEXT("%s / cash %s / company value %s / debt %s\nModel IQ %.2f / reputation %d / data quality %.1f%% / legal exposure %.1f%%\nDependency %.1f%% / employee care %.1f%% / automation %.1f%%"),*S(V.difficulty),*M(V.cash),*M(V.value),*M(V.debt),V.modelMicroIQ/1000000.,V.reputation,V.dataQualityBps/100.,V.legalBps/100.,V.dependencyBps/100.,V.employeeCareBps/100.,V.automationBps/100.),18);
    Text(Content,TEXT("Decisions that brought the company here"),22);for(const auto& D:E.decisions) Text(Content,S(D.action+": "+D.detail),14);
    Button(Content,TEXT("Start a new company"),TEXT("new"));auto* Saves=GetGameInstance()->GetSubsystem<UMaiSaveSubsystem>();
    Button(Content,TEXT("Return to a playable saved company"),TEXT("return-save"),E.allowReturnToSave && Saves && Saves->CanReturnToSave(Slot));
}
void UMaiFlowWidget::Refresh(){
    if(!Canvas) return;
    if(!Company || !Company->CampaignDomain()) {
        if(Signature!=TEXT("configuration-error")) {
            Signature=TEXT("configuration-error");Canvas->ClearChildren();
            auto* Error=WidgetTree->ConstructWidget<UBorder>();Error->SetBrushColor(FLinearColor(.018f,.022f,.028f,1));Error->SetPadding(FMargin(32));
            auto* Position=Canvas->AddChildToCanvas(Error);Position->SetAnchors(FAnchors(0,0,1,1));Position->SetOffsets(FMargin(0));
            Error->SetContent(Text(nullptr,TEXT("NEURON / configuration failed\n")+(Company?Company->LastMessage.ToString():TEXT("Company subsystem unavailable")),22));
        }
        return;
    }
    const auto& G=*Company->CampaignDomain();const auto& V=G.View();
    FString Key=FString::Printf(TEXT("%d/%d/%d/%lld/%lld/%d/%d/%d/%d/%d/%d/%d"),static_cast<int32>(V.screen),V.prologueStep,static_cast<int32>(V.loading.phase),static_cast<long long>(SelectedBatch),static_cast<long long>(V.inventory.sequence),static_cast<int32>(V.specialists.size()),V.ai.level,V.ai.created,V.overwork,V.ending.offerOnly,static_cast<int32>(V.ending.kind),G.Core().View().paused);
    for(const auto& B:V.inventory.batches) Key+=FString::Printf(TEXT("b%d"),static_cast<int32>(B.status));
    for(const auto& R:V.reviews) Key+=FString::Printf(TEXT("r%d/%d"),static_cast<int32>(R.phase),static_cast<int32>(R.decisions.size()));
    for(const auto& J:V.training) Key+=FString::Printf(TEXT("j%d"),static_cast<int32>(J.phase));
    if(Key!=Signature){Signature=Key;Build();}
    if(Summary) Summary->SetText(FText::FromString(FString::Printf(TEXT("%s / %s / %s / reputation %d / game hours %.3f"),*S(V.companyName),*S(V.difficulty),*M(G.Core().View().cash),G.Core().View().reputation,static_cast<double>(G.Core().View().now)/mai::Hour)));
    if(Feedback) Feedback->SetText(FText::FromString(Message.IsEmpty()?Company->LastMessage.ToString():Message));
    if(LoadingText){LoadingText->SetText(FText::FromString(S(V.loading.phase==mai::LoadPhase::Failed?V.loading.error:V.loading.operation)));if(Progress){Progress->SetVisibility(V.loading.progressBps<0?ESlateVisibility::Collapsed:ESlateVisibility::Visible);Progress->SetPercent(FMath::Max(0,V.loading.progressBps)/10000.f);}}
    if(JobText){
        FString T=FString::Printf(TEXT("Installed compute %.2f / available for training %.2f / model IQ %.4f / legal exposure %.1f%%\n"),G.Core().Economy().computeMilli/1000.,G.AvailableTrainingCompute()/1000.,V.modelMicroIQ/1000000.,V.legalExposureBps/100.);
        for(const auto& J:V.training) T+=FString::Printf(TEXT("Job #%lld / remaining %.3f units / expected IQ +%.4f / elapsed %.3f h / %s\n"),static_cast<long long>(J.id),J.remainingMicro/1000000.,J.expectedGainMicroIQ/1000000.,static_cast<double>(J.elapsed)/mai::Hour,J.phase==mai::JobPhase::Complete?TEXT("complete"):J.phase==mai::JobPhase::Paused?TEXT("paused"):G.AvailableTrainingCompute()>0?TEXT("running"):TEXT("waiting for compute"));
        for(const auto& R:V.reviews) if(R.method!=mai::ReviewMethod::Manual && R.phase!=mai::ReviewPhase::Complete) T+=FString::Printf(TEXT("Review #%lld: %.2f units remaining / %.3f game h\n"),static_cast<long long>(R.id),R.remainingMicro/1000000.,static_cast<double>(R.elapsed)/mai::Hour);
        JobText->SetText(FText::FromString(T));
    }
}
