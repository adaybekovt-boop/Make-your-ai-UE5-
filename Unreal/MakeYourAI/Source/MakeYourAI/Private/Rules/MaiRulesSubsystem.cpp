#include "Rules/MaiRulesSubsystem.h"
#include "MaiRulesJson.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Core/MaiGameInstance.h"
#include "Campaign/MaiLoadingSubsystem.h"
#include "World/MaiPlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Subsystems/SubsystemCollection.h"
#include <cmath>
using namespace MaiJson;
void UMaiRulesSubsystem::Initialize(FSubsystemCollectionBase& Collection){
    Super::Initialize(Collection);Collection.InitializeDependency<UMaiCompanySubsystem>();Company=GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    TArray<uint8> Bytes;FString ManifestText;const FString Root=FPaths::ProjectContentDir()/TEXT("Rules");
    if(!FFileHelper::LoadFileToArray(Bytes,*(Root/TEXT("mai-rules.js")))||!FFileHelper::LoadFileToString(ManifestText,*(Root/TEXT("mai-rules.manifest.json")))){LastError=TEXT("Не удалось загрузить игровые правила. Проверьте установку игры.");return;}
    uint8 Hash[20];FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash);const auto Manifest=Decode(ManifestText);
    if(!Manifest||String(Manifest,TEXT("bundleSha1"))!=BytesToHex(Hash,20).ToLower()){LastError=TEXT("Игровые файлы повреждены. Сохранения не изменены.");return;}
    VM=MakeUnique<mai::RulesVM>();std::string Error;const auto* Instance=Cast<UMaiGameInstance>(GetGameInstance());
    const uint32 Seed=Instance&&Instance->SessionSeed?uint32(Instance->SessionSeed):1296124209u;
    if(!VM->Open(std::string(reinterpret_cast<const char*>(Bytes.GetData()),Bytes.Num()),Seed,Error)){LastError=TEXT("Не удалось запустить игровые правила.");UE_LOG(LogTemp,Error,TEXT("Native rules startup: %s"),UTF8_TO_TCHAR(Error.c_str()));return;}
    Invoke(Request(TEXT("state")),State);bHasSave=FPaths::FileExists(SlotPath(TEXT("campaign")))||FPaths::FileExists(SlotPath(TEXT("autosave")));
}
void UMaiRulesSubsystem::Deinitialize(){if(IsReady()&&!bLoadingTransaction&&String(State,TEXT("phase"))==TEXT("playing"))SaveSlot(TEXT("autosave"));VM.Reset();Company=nullptr;Super::Deinitialize();}
bool UMaiRulesSubsystem::Invoke(const TSharedRef<FJsonObject>& R,TSharedPtr<FJsonObject>& Value){
    if(!IsReady())return false;std::string Response,Error;const FTCHARToUTF8 Input(*Encode(R));
    if(!VM->Call(std::string(Input.Get(),Input.Length()),Response,Error)){LastError=TEXT("Операция прервана. Состояние игры не подтверждено.");UE_LOG(LogTemp,Error,TEXT("Native rules call: %s"),UTF8_TO_TCHAR(Error.c_str()));return false;}
    const auto Envelope=Decode(UTF8_TO_TCHAR(Response.c_str()));if(!Envelope){LastError=TEXT("Некорректный ответ игровых правил.");return false;}
    Value=Object(Envelope,TEXT("value"));if(Value&&Value->HasField(TEXT("company")))State=Value;
    if(!Boolean(Envelope,TEXT("ok"))){LastError=String(Envelope,TEXT("error"),TEXT("Действие недоступно."));return false;}
    return true;
}
bool UMaiRulesSubsystem::Command(const FString& Action,const TArray<TSharedPtr<FJsonValue>>& Args){auto R=Request(TEXT("command"));R->SetStringField(TEXT("action"),Action);R->SetArrayField(TEXT("args"),Args);TSharedPtr<FJsonObject> Out;return Invoke(R,Out);}
bool UMaiRulesSubsystem::Dispatch(const FString& Action,const TArray<TSharedPtr<FJsonValue>>& Args){
    if(!IsReady()||!Company||!Company->Campaign())return false;LastError.Reset();
    if(Action.StartsWith(TEXT("host:")))return HostAction(Action.Mid(5),Args);
    if(bLoadingTransaction){LastError=TEXT("Дождитесь завершения загрузки.");return false;}
    if(Action==TEXT("startTraining")&&!SyncReviews())return false;
    const auto Backup=Capture();
    if(!Command(Action,Args))return false;
    if(!Project()||!SyncReviews()){const FString Why=LastError;if(Backup)ApplyCapture(Backup);LastError=Why;return false;}
    Company->OnChanged.Broadcast();return true;
}
bool UMaiRulesSubsystem::Project(double ElapsedHours){
    auto* C=Company?Company->Campaign():nullptr;if(!C||!State)return false;
    if(C->View().difficulty.empty()||C->View().prologueStep!=3)return true;
    const auto Root=Object(State,TEXT("company")),Ledger=Object(Root,TEXT("company"));if(!Root||!Ledger){LastError=TEXT("Компания недоступна.");return false;}
    for(const TCHAR* Field:{TEXT("cash"),TEXT("totalCapex"),TEXT("totalExpenses"),TEXT("totalRevenue")})if(!FMath::IsFinite(Number(Ledger,Field))||FMath::Abs(Number(Ledger,Field))>double(mai::MoneyLimit)/mai::Unit){LastError=TEXT("Значение компании превышает диапазон совместимости нативной кампании. Операция отменена.");return false;}
    if(Number(Ledger,TEXT("elapsedGameHours"))>1e9||Number(State,TEXT("learnedIQ"))>1e6||Number(State,TEXT("users"))>1e8){LastError=TEXT("Значение вне диапазона нативной кампании.");return false;}
    auto P=C->Core().View();const auto Money=[](double Value){return static_cast<mai::Money>(std::llround(Value*mai::Unit));};
    P.cash=Money(Number(Ledger,TEXT("cash")));P.capex=Money(Number(Ledger,TEXT("totalCapex")));P.expenses=Money(Number(Ledger,TEXT("totalExpenses")));P.revenue=Money(Number(Ledger,TEXT("totalRevenue")));
    P.now=static_cast<mai::Tick>(std::llround(Number(Ledger,TEXT("elapsedGameHours"))*mai::Hour));P.paused=Boolean(Ledger,TEXT("paused"));P.speed=int(Number(Ledger,TEXT("speed"),1));P.reputation=int(Number(Ledger,TEXT("reputation"),50));
    P.usersMicro=Money(Number(State,TEXT("users")));P.dirtyHistory=Boolean(Root,TEXT("dataLiability"));P.expenseCarry=0;P.revenueCarry=0;
    const auto Investors=Object(Ledger,TEXT("investors"));P.restrictedUntil=static_cast<mai::Tick>(std::llround(Number(Investors,TEXT("restrictedUntil"))*mai::Hour));
    P.unlockedRegions={"home"};for(const auto& R:Array(Ledger,TEXT("regions")))P.unlockedRegions.push_back(TCHAR_TO_UTF8(*R->AsString()));
    P.orders.clear();P.orderSequence=static_cast<int64>(Number(Ledger,TEXT("orderSeq")));
    auto Locations=Array(Ledger,TEXT("locations"));Locations.Append(Array(Ledger,TEXT("regionLocations")));
    for(const auto& V:Locations){const auto L=V->AsObject();const FString Id=String(L,TEXT("id"));const int Index=C->Core().Definitions().LocationIndex(TCHAR_TO_UTF8(*Id));if(Index<0){LastError=TEXT("Неизвестная площадка в проекции компании.");return false;}
        auto& Target=P.locations[Index];const auto& Definition=C->Core().Definitions().locations[Index];Target.owned=Boolean(L,TEXT("owned"));Target.serverSequence=static_cast<int64>(Number(L,TEXT("serverSeq")));Target.chassisStock={};Target.chipStock={};Target.slots.assign(Definition.rows*Definition.cols,mai::Slot{});
        const auto Inventory=Object(L,TEXT("inventory"));
        for(int I=0;I<3;++I){const FString Key=UTF8_TO_TCHAR(C->Core().Definitions().chassis[I].id.c_str());Target.chassisStock[I].total=int(Number(Object(Inventory,TEXT("chassis")),*Key));Target.chassisStock[I].grey=int(Number(Object(Inventory,TEXT("greyChassis")),*Key));}
        for(int I=0;I<4;++I){const FString Key=UTF8_TO_TCHAR(C->Core().Definitions().chips[I].id.c_str());Target.chipStock[I].total=int(Number(Object(Inventory,TEXT("chips")),*Key));Target.chipStock[I].grey=int(Number(Object(Inventory,TEXT("greyChips")),*Key));}
        const auto Place=[&](const TSharedPtr<FJsonObject>& Item,bool Server)->bool{
            const auto Position=Object(Item,TEXT("gridPosition"));if(!Position)return true;const int Row=int(Number(Position,TEXT("row"))),Col=int(Number(Position,TEXT("col")));if(Row<0||Col<0||Row>=Definition.rows||Col>=Definition.cols)return false;
            auto& Slot=Target.slots[Row*Definition.cols+Col];if(Slot.chassis>=0)return false;
            FString Chassis=String(Item,TEXT("chassis"));const FString Chip=String(Item,TEXT("chip"));
            if(Chassis.IsEmpty())Chassis=Chip==TEXT("flagship")?TEXT("rack-enterprise"):Chip==TEXT("accelerator")?TEXT("rack-cooled"):TEXT("rack-basic");
            for(int I=0;I<3;++I)if(Chassis==UTF8_TO_TCHAR(C->Core().Definitions().chassis[I].id.c_str()))Slot.chassis=I;
            if(Server){for(int I=0;I<4;++I)if(Chip==UTF8_TO_TCHAR(C->Core().Definitions().chips[I].id.c_str()))Slot.chip=I;Slot.serverId=FCString::Atoi64(*String(Item,TEXT("id")).Mid(7));Slot.overclockMilli=int(std::llround(Number(Item,TEXT("overclock"),1)*1000));Target.serverSequence=FMath::Max(Target.serverSequence,Slot.serverId);}
            return Slot.chassis>=0&&(!Server||Slot.chip>=0);
        };
        for(const auto& Rig:Array(L,TEXT("rigs")))if(!Place(Rig->AsObject(),false)){LastError=TEXT("Ошибка стойки в сохранении.");return false;}
        for(const auto& Server:Array(L,TEXT("installedServers")))if(!Place(Server->AsObject(),true)){LastError=TEXT("Ошибка оборудования в сохранении.");return false;}
    }
    const auto Result=C->SynchronizeBrowser(P,static_cast<int64>(std::llround(Number(State,TEXT("learnedIQ"))*1000000)),static_cast<mai::Tick>(std::llround(ElapsedHours*mai::Hour)),String(Ledger,TEXT("ending"))==TEXT("acquired"));
    if(!Result.ok){LastError=UTF8_TO_TCHAR(Result.message.c_str());return false;}
    // Review completion may change reputation. Feed that delta back exactly once,
    // never overwrite it with the next browser projection.
    const auto& After=C->Core().View();if(After.reputation!=P.reputation){auto Change=Request(TEXT("native-ledger"));Change->SetNumberField(TEXT("reputation"),After.reputation-P.reputation);TSharedPtr<FJsonObject> Out;if(!Invoke(Change,Out))return false;}
    return true;
}
bool UMaiRulesSubsystem::SyncReviews(){
    auto* C=Company?Company->Campaign():nullptr;if(!C||!State)return false;if(!C->CanPlay()&&C->View().prologueStep!=3)return true;
    TArray<TSharedPtr<FJsonValue>> Verified;
    for(const auto& Value:Array(Object(State,TEXT("company")),TEXT("models"))){const auto Model=Value->AsObject();const FString ModelId=String(Model,TEXT("id"));
        for(const auto& LotValue:Array(Object(Model,TEXT("state")),TEXT("queue"))){const auto Lot=LotValue->AsObject();const FString Key=ModelId+TEXT(":")+FString::Printf(TEXT("%lld"),static_cast<int64>(Number(Lot,TEXT("id"))));
            if(!ReviewLinks.Contains(Key)){if(!C->CanPlay()){LastError=TEXT("Нельзя добавить проверку в завершённую компанию.");return false;}const auto R=C->ReceivePaidDatasetSample(String(Lot,TEXT("quality"))==TEXT("official")?"official-text-100":"unofficial-text-100");if(!R.ok){LastError=UTF8_TO_TCHAR(R.message.c_str());return false;}ReviewLinks.Add(Key,C->View().inventory.sequence);}
            const auto* Batch=C->Batch(ReviewLinks[Key]);if(!Batch){LastError=TEXT("История проверки данных повреждена.");return false;}
            if(Batch->status==mai::DatasetStatus::Verified||Batch->status==mai::DatasetStatus::Trained)Verified.Add(S(Key));
        }
    }
    double Reservation=0;for(const auto& Review:C->View().reviews)if(Review.method==mai::ReviewMethod::AI&&Review.phase!=mai::ReviewPhase::Complete){Reservation=FMath::Min(double(C->View().ai.computeMilli),double(C->Core().Economy().computeMilli))/1000.;break;}
    auto RequestData=Request(TEXT("sync-review"));RequestData->SetArrayField(TEXT("verified"),Verified);RequestData->SetNumberField(TEXT("reservedCompute"),Reservation);TSharedPtr<FJsonObject> Out;return Invoke(RequestData,Out);
}
bool UMaiRulesSubsystem::NativeAction(TFunctionRef<mai::Result(mai::Campaign&)> Action){
    if(!Company||!Company->Campaign()||!Project())return false;auto* C=Company->Campaign();const auto Before=C->Core().View();const std::string BeforeCampaign=C->Save();
    const auto Result=Company->CampaignTransact(Action);if(!Result.bSuccess){LastError=Result.Message.ToString();return false;}
    const auto After=C->Core().View();auto R=Request(TEXT("native-ledger"));R->SetNumberField(TEXT("cash"),double(After.cash-Before.cash)/mai::Unit);R->SetNumberField(TEXT("capex"),double(After.capex-Before.capex)/mai::Unit);R->SetNumberField(TEXT("expenses"),double(After.expenses-Before.expenses)/mai::Unit);R->SetNumberField(TEXT("revenue"),double(After.revenue-Before.revenue)/mai::Unit);R->SetNumberField(TEXT("reputation"),After.reputation-Before.reputation);
    TSharedPtr<FJsonObject> Out;if(!Invoke(R,Out)){C->Load(BeforeCampaign);return false;}return Project()&&SyncReviews();
}
void UMaiRulesSubsystem::SetForeground(bool Foreground){if(bForeground==Foreground)return;bForeground=Foreground;FrameCarry=0;bSkipForegroundFrame=Foreground;if(!Foreground&&IsReady()&&!bLoadingTransaction)SaveSlot(TEXT("autosave"));}
void UMaiRulesSubsystem::AdvanceFrame(float Seconds){
    CheckRestore();if(!IsReady()||!Company||!Company->Campaign()||!bForeground||bLoadingTransaction||!FMath::IsFinite(Seconds)||Seconds<=0)return;
    if(bSkipForegroundFrame){bSkipForegroundFrame=false;return;}
    auto* C=Company->Campaign();if(!C->CanPlay()||String(State,TEXT("phase"))!=TEXT("playing"))return;
    AutoSaveCarry+=Seconds;FrameCarry+=FMath::Min(Seconds,2.f);if(FrameCarry<.25f)return;const float Delta=FMath::Min(FrameCarry,2.f);FrameCarry=0;
    if(!SyncReviews())return;const auto Old=Capture();const double Before=Number(Object(Object(State,TEXT("company")),TEXT("company")),TEXT("elapsedGameHours"));
    auto R=Request(TEXT("tick"));R->SetNumberField(TEXT("seconds"),Delta);R->SetNumberField(TEXT("reservedCompute"),Number(State,TEXT("reservedCompute")));TSharedPtr<FJsonObject> Out;
    if(!Invoke(R,Out))return;const double After=Number(Object(Object(State,TEXT("company")),TEXT("company")),TEXT("elapsedGameHours"));
    if(!Project(After-Before)||!SyncReviews()){const FString Why=LastError;if(Old)ApplyCapture(Old);LastError=Why;return;}
    Company->OnChanged.Broadcast();if(AutoSaveCarry>=15){AutoSaveCarry=0;SaveSlot(TEXT("autosave"));}
}
TSharedPtr<FJsonObject> UMaiRulesSubsystem::ViewModel(){
    if(!IsReady()){auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("mode"),TEXT("full"));auto C=Node(TEXT("column"),TEXT("startup-error"));Add(C,Head(TEXT("startup-title"),TEXT("Не удалось открыть игру")));Add(C,Text(TEXT("startup-error-text"),LastError));V->SetObjectField(TEXT("content"),C);return V;}
    auto R=Request(TEXT("view"));R->SetObjectField(TEXT("host"),HostView());TSharedPtr<FJsonObject> Value;return Invoke(R,Value)?Value:nullptr;
}
