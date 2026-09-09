#include "Rules/MaiRulesSubsystem.h"
#include "World/MaiCityLighting.h"
#include "MaiRulesJson.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Campaign/MaiLoadingSubsystem.h"
#include "World/MaiPlayerController.h"
#include "World/MaiWalkCharacter.h"
#include "Interaction/MaiInteriorPoint.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "AudioDevice.h"
#include "Misc/ConfigCacheIni.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/UserInterfaceSettings.h"
using namespace MaiJson;
namespace {
FString Utf(const std::string& S){return UTF8_TO_TCHAR(S.c_str());}
FString Dollars(mai::Money M){return Utf(mai::FormatMoney(M));}
}
TSharedPtr<FJsonObject> UMaiRulesSubsystem::HostView(){
    auto H=MakeShared<FJsonObject>();H->SetBoolField(TEXT("hasSave"),bHasSave);H->SetStringField(TEXT("saveError"),LastError);
    auto* C=Company?Company->Campaign():nullptr;if(!C)return H;const auto& G=C->View();H->SetStringField(TEXT("companyName"),Utf(G.companyName));H->SetBoolField(TEXT("walking"),!G.interior.empty()&&C->CanPlay());H->SetBoolField(TEXT("canReturn"),bHasSave&&G.ending.allowReturnToSave);
    if(!G.interior.empty()){
        FString Hint=TEXT("WASD — движение · мышь — осмотр\nTab — курсор / осмотр · E — взаимодействие");
        if(auto* Walker=Cast<AMaiWalkCharacter>(UGameplayStatics::GetPlayerPawn(GetGameInstance(),0))){
            if(auto* Focused=Walker->FocusedInteraction())Hint+=TEXT("\nE — ")+Focused->InteractionLabel();}
        H->SetStringField(TEXT("inputHint"),Hint);
    }
    if(G.screen==mai::Screen::Loading){auto L=MakeShared<FJsonObject>();L->SetStringField(TEXT("status"),G.loading.phase==mai::LoadPhase::Failed?Utf(G.loading.error):TEXT("Подготовка игрового мира…"));
        if(G.loading.progressBps<0)L->SetField(TEXT("progress"),MakeShared<FJsonValueNull>());else L->SetNumberField(TEXT("progress"),G.loading.progressBps/10000.);
        const auto* Loader=GetGameInstance()->GetSubsystem<UMaiLoadingSubsystem>();L->SetBoolField(TEXT("cancel"),G.prologueStep==3&&Loader&&Loader->CanCancel());L->SetBoolField(TEXT("failed"),G.loading.phase==mai::LoadPhase::Failed);H->SetObjectField(TEXT("loading"),L);return H;}
    TSharedPtr<FJsonObject> Overlay;
    if(G.screen==mai::Screen::Difficulty){Overlay=Node(TEXT("column"),TEXT("difficulty"));Add(Overlay.ToSharedRef(),Head(TEXT("difficulty-title"),TEXT("Сложность")));Add(Overlay.ToSharedRef(),Text(TEXT("difficulty-rule"),TEXT("Сложность фиксируется на всю партию.")));
        const double BaseCapital=Number(Object(State,TEXT("company")),TEXT("startingCapital"),12000.);
        for(const auto& D:C->Rules().difficulties){auto Card=Node(TEXT("card"),Utf(D.id));Add(Card,Head(Utf(D.id)+TEXT("-title"),D.id=="Easy"?TEXT("Легко"):D.id=="Hard"?TEXT("Сложно"):TEXT("Обычно")));Add(Card,Text(Utf(D.id)+TEXT("-capital"),TEXT("Начальный капитал: ")+Dollars(static_cast<mai::Money>(BaseCapital*mai::Unit)*D.capitalBps/10000)));Add(Card,Button(Utf(D.id)+TEXT("-choose"),TEXT("Выбрать"),TEXT("host:difficulty"),{S(Utf(D.id))}));Add(Overlay.ToSharedRef(),Card);}}
    if(G.screen==mai::Screen::Prologue){Overlay=Node(TEXT("column"),TEXT("prologue"));Add(Overlay.ToSharedRef(),Head(TEXT("prologue-title"),TEXT("Всё начинается с гаража")));
        if(G.prologueStep==0){auto Name=Node(TEXT("input"),TEXT("company-name"),TEXT("Название компании"));Name->SetStringField(TEXT("action"),TEXT("ui:field"));Name->SetArrayField(TEXT("args"),{S(TEXT("companyName"))});Name->SetStringField(TEXT("value"),String(Object(Object(State,TEXT("ui")),TEXT("fields")),TEXT("companyName"),String(Object(Object(State,TEXT("ui")),TEXT("fields")),TEXT("name"))));Add(Overlay.ToSharedRef(),Name);Add(Overlay.ToSharedRef(),Button(TEXT("prologue-create"),TEXT("Основать компанию"),TEXT("host:prologue-name")));}
        else if(G.prologueStep==1){Add(Overlay.ToSharedRef(),Text(TEXT("prologue-budget"),TEXT("Площадка, оборудование, данные и сотрудники оплачиваются отдельно. Заказ не устанавливается мгновенно: дождитесь доставки, затем используйте склад.")));Add(Overlay.ToSharedRef(),Button(TEXT("prologue-budget-next"),TEXT("Бюджет понятен"),TEXT("host:prologue-budget")));}
        else {Add(Overlay.ToSharedRef(),Text(TEXT("prologue-task"),TEXT("Купите гараж, закажите и установите первый сервер. У рабочего стола проверьте данные и запустите обучение.")));Add(Overlay.ToSharedRef(),Button(TEXT("prologue-play"),TEXT("Начать"),TEXT("host:prologue-start")));}}
    if((G.screen==mai::Screen::Ending||G.screen==mai::Screen::Results)&&String(State,TEXT("phase"))!=TEXT("setup")){Overlay=Node(TEXT("column"),TEXT("campaign-ending"));Add(Overlay.ToSharedRef(),Head(TEXT("ending-title"),Utf(G.ending.title)));Add(Overlay.ToSharedRef(),Text(TEXT("ending-scene"),Utf(G.ending.line)));
        const auto L=Object(Object(State,TEXT("company")),TEXT("company"));Add(Overlay.ToSharedRef(),Text(TEXT("ending-cash"),FString::Printf(TEXT("Баланс $%.0f · Выручка $%.0f · Расходы $%.0f · IQ %.1f"),Number(L,TEXT("cash")),Number(L,TEXT("totalRevenue")),Number(L,TEXT("totalExpenses")),Number(State,TEXT("learnedIQ")))));
        for(size_t I=0;I<G.ending.decisions.size();++I){const auto& D=G.ending.decisions[I];Add(Overlay.ToSharedRef(),Text(FString::Printf(TEXT("decision-%llu"),static_cast<unsigned long long>(I)),Utf(D.action)+TEXT(" · ")+Utf(D.detail)));}
        if(G.ending.offerOnly){Add(Overlay.ToSharedRef(),Button(TEXT("ending-accept"),TEXT("Принять предложение"),TEXT("host:end-accept")));Add(Overlay.ToSharedRef(),Button(TEXT("ending-decline"),TEXT("Сохранить независимость"),TEXT("host:end-decline")));}
        Add(Overlay.ToSharedRef(),Button(TEXT("ending-new"),TEXT("Новая игра"),TEXT("host:new")));Add(Overlay.ToSharedRef(),Button(TEXT("ending-return"),TEXT("Вернуться к сохранению"),TEXT("host:load"),{},bHasSave&&G.ending.allowReturnToSave));Add(Overlay.ToSharedRef(),Button(TEXT("ending-menu"),TEXT("Главное меню"),TEXT("host:menu")));}
    if(bReviewOpen&&C->CanPlay())Overlay=ReviewView();
    if(Overlay){H->SetObjectField(TEXT("overlay"),Overlay);H->SetStringField(TEXT("screen"),Utf(mai::ScreenName(G.screen)));}return H;
}
TSharedPtr<FJsonObject> UMaiRulesSubsystem::ReviewView()const{
    auto Root=Node(TEXT("column"),TEXT("reviews"));const auto* C=Company?Company->Campaign():nullptr;if(!C)return Root;
    Add(Root,Head(TEXT("review-title"),TEXT("Проверка данных")));Add(Root,Text(TEXT("review-explanation"),TEXT("Проверка принадлежит купленной партии. Она не покупает данные повторно и не меняет их происхождение.")));
    const auto& G=C->View();
    for(const auto& B:G.inventory.batches){auto Card=Node(TEXT("card"),FString::Printf(TEXT("review-batch-%lld"),B.id));Add(Card,Head(FString::Printf(TEXT("batch-title-%lld"),B.id),FString::Printf(TEXT("Партия #%lld · %d единиц"),B.id,B.volume)));Add(Card,Text(FString::Printf(TEXT("batch-status-%lld"),B.id),Utf(mai::DatasetStatusName(B.status))+FString::Printf(TEXT(" · Качество %.0f%% · Правовой риск %.0f%%"),B.quality.scoreBps/100.,B.quality.legalBps/100.)));
        if(B.status==mai::DatasetStatus::Unreviewed){for(int Method=1;Method<=3;++Method)Add(Card,Button(FString::Printf(TEXT("batch-%lld-method-%d"),B.id,Method),Method==1?TEXT("Manual review"):Method==2?TEXT("Human review"):TEXT("AI review"),TEXT("host:review-start"),{N(double(B.id)),N(Method)}));}
        const auto* R=C->Review(B.reviewId);if(R&&R->phase!=mai::ReviewPhase::Complete){
            if(R->method==mai::ReviewMethod::Manual&&R->decisions.size()<R->items.size()){const auto& Item=R->items[R->decisions.size()];Add(Card,Head(FString::Printf(TEXT("manual-prompt-%lld"),B.id),Utf(Item.prompt)));Add(Card,Text(FString::Printf(TEXT("manual-left-%lld"),B.id),Utf(Item.left)));Add(Card,Text(FString::Printf(TEXT("manual-right-%lld"),B.id),Utf(Item.right)));
                for(int Side=0;Side<3;++Side)Add(Card,Button(FString::Printf(TEXT("manual-choice-%lld-%d"),B.id,Side),Side==0?TEXT("Левый ответ лучше"):Side==1?TEXT("Правый ответ лучше"):TEXT("Оба ответа плохие"),TEXT("host:review-choice"),{N(double(R->id)),N(Side)}));
                Add(Card,Button(FString::Printf(TEXT("manual-skip-%lld"),B.id),TEXT("Пропустить"),TEXT("host:review-skip"),{N(double(R->id))}));Add(Card,Button(FString::Printf(TEXT("manual-cancel-%lld"),B.id),TEXT("Отменить проверку"),TEXT("host:review-cancel"),{N(double(R->id))}));
            }else{auto P=Node(TEXT("progress"),FString::Printf(TEXT("review-progress-%lld"),R->id),TEXT("Проверка выполняется"));P->SetNumberField(TEXT("value"),1.-double(R->remainingMicro)/(B.volume*1000000.));Add(Card,P);}}
        Add(Root,Card);
    }
    Add(Root,Button(TEXT("hire-reviewer"),TEXT("Нанять специалиста · ")+Dollars(C->Rules().humanHire*(C->Difficulty()?C->Difficulty()->hiringBps:10000)/10000),TEXT("host:review-hire"),{},G.specialists.size()<4));
    Add(Root,Button(TEXT("create-review-ai"),TEXT("Создать AI-reviewer · ")+Dollars(C->Rules().aiSetup),TEXT("host:review-ai"),{},!G.ai.created));
    Add(Root,Button(TEXT("upgrade-review-ai"),TEXT("Улучшить AI-reviewer · ")+Dollars(C->Rules().aiUpgrade),TEXT("host:review-upgrade"),{},G.ai.created&&G.ai.level<5));
    Add(Root,Button(TEXT("review-overwork"),G.overwork?TEXT("Отменить переработки проверяющих"):TEXT("Переработки проверяющих"),TEXT("host:review-overwork")));
    Add(Root,Text(TEXT("review-fees"),TEXT("Специалист получает оплату за партию. AI-review резервирует мощность и может систематически ошибаться.")));
    if(G.warnings>G.ignoredWarnings){Add(Root,Button(TEXT("remediate"),TEXT("Исправить правовой риск"),TEXT("host:remediate")));Add(Root,Button(TEXT("ignore-warning"),TEXT("Игнорировать предупреждение"),TEXT("host:ignore-warning")));}
    Add(Root,Button(TEXT("review-back"),TEXT("Вернуться к обучению"),TEXT("host:review-close")));return Root;
}
bool UMaiRulesSubsystem::HostAction(const FString& Action,const TArray<TSharedPtr<FJsonValue>>& Args){
    auto* C=Company?Company->Campaign():nullptr;auto* PC=Cast<AMaiPlayerController>(UGameplayStatics::GetPlayerController(GetGameInstance(),0));if(!C)return false;
    const auto Arg=[&](int I){return Args.IsValidIndex(I)?Args[I]->AsString():FString();};const auto Num=[&](int I){return Args.IsValidIndex(I)?Args[I]->AsNumber():0.;};
    const auto Flow=[&](TFunctionRef<mai::Result(mai::Campaign&)> F){const auto R=Company->CampaignTransact(F);if(!R.bSuccess)LastError=R.Message.ToString();return R.bSuccess;};
    if(Action==TEXT("end-accept"))return NativeAction([](mai::Campaign& G){return G.ChooseEnding(mai::EndingKind::Acquisition);});
    if(Action==TEXT("end-decline"))return NativeAction([](mai::Campaign& G){return G.DeclineSale();});
    if(Action==TEXT("save"))return SaveSlot(TEXT("campaign"));if(Action==TEXT("load"))return LoadSlot(TEXT("campaign"));
    if(Action==TEXT("new"))return Command(TEXT("beginSetup"),{});
    if(Action==TEXT("setup-next")){
        const FString Name=String(Object(Object(State,TEXT("ui")),TEXT("fields")),TEXT("name")).TrimStartAndEnd();if(Name.IsEmpty()||Name.Len()>48){LastError=TEXT("Введите название модели: 1–48 символов.");return false;}
        // BeginNewGame is destructive only after explicit setup confirmation; the browser draft itself is not.
        if(C->View().screen!=mai::Screen::MainMenu&&C->View().screen!=mai::Screen::Ending&&C->View().screen!=mai::Screen::Results)if(!Flow([](mai::Campaign& G){return G.ShowScreen(mai::Screen::MainMenu);}))return false;
        if(!Flow([](mai::Campaign& G){return G.BeginNewGame();}))return false;ReviewLinks.Reset();return Flow([](mai::Campaign& G){return G.ShowDifficulty();});
    }
    if(Action==TEXT("difficulty"))return Flow([&](mai::Campaign& G){return G.ChooseDifficulty(TCHAR_TO_UTF8(*Arg(0)));});
    if(Action==TEXT("prologue-name")){FString Name=String(Object(Object(State,TEXT("ui")),TEXT("fields")),TEXT("companyName"),String(Object(Object(State,TEXT("ui")),TEXT("fields")),TEXT("name")));FTCHARToUTF8 Encoded(*Name);if(Encoded.Length()>80){LastError=TEXT("Название компании в прологе превышает 80 байт UTF-8.");return false;}return Flow([&](mai::Campaign& G){return G.PrologueAction(TCHAR_TO_UTF8(*Name));});}
    if(Action==TEXT("prologue-budget"))return Flow([](mai::Campaign& G){return G.PrologueAction("budget");});
    if(Action==TEXT("prologue-start")){
        const auto Capital=C->Core().View().cash;const double BaseCapital=Number(Object(State,TEXT("company")),TEXT("startingCapital"),Number(Object(Object(State,TEXT("company")),TEXT("company")),TEXT("cash"),12000.));if(!Command(TEXT("ui:start"),{}))return false;
        const double Delta=double(Capital)/mai::Unit-BaseCapital;auto R=Request(TEXT("native-ledger"));R->SetNumberField(TEXT("cash"),Delta);R->SetNumberField(TEXT("startingCapital"),Delta);TSharedPtr<FJsonObject> Out;if(!Invoke(R,Out))return false;
        if(!Flow([](mai::Campaign& G){return G.PrologueAction("accept-task");}))return false;return Project();
    }
    if(Action==TEXT("menu")){if(!Flow([](mai::Campaign& G){return G.ShowScreen(mai::Screen::MainMenu);}))return false;bReviewOpen=false;return Command(TEXT("cancelSetup"),{});}
    if(Action==TEXT("quit")){if(String(State,TEXT("phase"))==TEXT("playing")&&!SaveSlot(TEXT("autosave")))return false;UKismetSystemLibrary::QuitGame(GetGameInstance(),PC,EQuitPreference::Quit,false);return true;}
    if(Action==TEXT("cancel")){auto* L=GetGameInstance()->GetSubsystem<UMaiLoadingSubsystem>();if(!L||!L->CanCancel()){LastError=TEXT("Мир уже активируется; эту фазу нельзя отменить.");return false;}L->Invalidate();bRestoreCancelled=bLoadingTransaction;const bool Ok=Flow([](mai::Campaign& G){return G.CancelLoad();});CheckRestore();return Ok;}
    if(Action==TEXT("retry")){if(auto* L=GetGameInstance()->GetSubsystem<UMaiLoadingSubsystem>()){L->Retry();return true;}return false;}
    if(Action==TEXT("enter")){
        if(!PC||Arg(0).IsEmpty()||!Project())return false;
        if(!Flow([&](mai::Campaign& G){return G.BeginLoad(mai::Screen::Gameplay,TCHAR_TO_UTF8(*Arg(0)));}))return false;
        // Entering from equipment must dismiss that panel, select the actual room
        // and expose the first-person view rather than leave a full-screen grid.
        return Command(TEXT("ui:location"),{S(Arg(0))});
    }
    if(Action==TEXT("leave")){if(PC){bReviewOpen=false;PC->ShowCity();return true;}return false;}
    if(Action==TEXT("review")){if(!C->CanPlay()){LastError=TEXT("Сначала начните партию.");return false;}bReviewOpen=true;return true;}
    if(Action==TEXT("review-close")){bReviewOpen=false;return Command(TEXT("ui:page"),{S(TEXT("training"))});}
    if(Action==TEXT("review-start")){if(int(Num(1))==1&&(!PC||!PC->AtReviewDesk())){LastError=TEXT("Для ручной проверки подойдите к рабочему столу в гараже.");return false;}return NativeAction([&](mai::Campaign& G){return G.StartReview(int64(Num(0)),static_cast<mai::ReviewMethod>(int(Num(1))));});}
    if(Action==TEXT("review-choice")){if(!PC||!PC->AtReviewDesk()){LastError=TEXT("Подойдите к рабочему столу.");return false;}return NativeAction([&](mai::Campaign& G){return G.ChooseReview(int64(Num(0)),int(Num(1)));});}
    if(Action==TEXT("review-skip"))return NativeAction([&](mai::Campaign& G){return G.SkipReview(int64(Num(0)));});
    if(Action==TEXT("review-cancel"))return NativeAction([&](mai::Campaign& G){return G.CancelReview(int64(Num(0)));});
    if(Action==TEXT("review-hire"))return NativeAction([](mai::Campaign& G){return G.HireSpecialist();});
    if(Action==TEXT("review-ai"))return NativeAction([](mai::Campaign& G){return G.CreateAIReviewer();});
    if(Action==TEXT("review-upgrade"))return NativeAction([](mai::Campaign& G){return G.ImproveAIReviewer();});
    if(Action==TEXT("review-overwork"))return NativeAction([](mai::Campaign& G){return G.SetOverwork(!G.View().overwork);});
    if(Action==TEXT("remediate"))return NativeAction([](mai::Campaign& G){return G.Remediate();});
    if(Action==TEXT("ignore-warning"))return NativeAction([](mai::Campaign& G){return G.IgnoreWarning();});
    if(Action==TEXT("start-training")){if(!PC||!PC->AtReviewDesk()){LastError=TEXT("Запустите обучение у рабочего стола в гараже.");return false;}if(!SyncReviews())return false;return Dispatch(TEXT("startTraining"));}
    if(Action==TEXT("volume")){Volume=FMath::Clamp(float(Num(0))/100,0.f,1.f);if(GetWorld()){auto Device=GetWorld()->GetAudioDevice();if(Device.IsValid())Device->SetTransientPrimaryVolume(Volume);}Command(TEXT("ui:field"),{S(TEXT("volume")),S(FString::FromInt(FMath::RoundToInt(Volume*100)))});GConfig->SetFloat(TEXT("MakeYourAI.Settings"),TEXT("Volume"),Volume,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);return true;}
    if(Action==TEXT("scale")){InterfaceScale=FMath::Clamp(float(Num(0))/100,.85f,1.4f);GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale=InterfaceScale;Command(TEXT("ui:field"),{S(TEXT("scale")),S(FString::FromInt(FMath::RoundToInt(InterfaceScale*100)))});GConfig->SetFloat(TEXT("MakeYourAI.Settings"),TEXT("Scale"),InterfaceScale,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);return true;}
    if(Action==TEXT("fullscreen")){if(GEngine&&GEngine->GetGameUserSettings()){auto* S=GEngine->GetGameUserSettings();S->SetFullscreenMode(S->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);S->ApplySettings(false);return true;}return false;}
    if(Action==TEXT("daynight")){for(TActorIterator<AMaiCityLighting> It(GetWorld());It;++It){It->SetActorTickEnabled(false);It->ApplyPreview(!It->PreviewNight);return true;}LastError=TEXT("Освещение города ещё не загружено.");return false;}
    if(Action==TEXT("auction")||Action==TEXT("nuclear")||Action==TEXT("greenhaven")){LastError=TEXT("Для этой локации в браузерном эталоне нет игровых правил. Покупка не выполнялась.");return false;}
    if(Action==TEXT("office")){LastError=TEXT("Покупка офиса и управление компанией доступны; прогулка по авторскому офису ещё не подключена.");return false;}
    LastError=TEXT("Действие не подключено: ")+Action;return false;
}
