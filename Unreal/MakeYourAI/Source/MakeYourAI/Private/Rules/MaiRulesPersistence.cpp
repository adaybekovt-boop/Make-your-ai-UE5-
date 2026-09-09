#include "Rules/MaiRulesSubsystem.h"
#include "Rules/MaiDurableFile.h"
#include "MaiRulesJson.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Campaign/MaiLoadingSubsystem.h"
#include "World/MaiPlayerController.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Misc/SecureHash.h"
#include "Kismet/GameplayStatics.h"
using namespace MaiJson;
namespace {
FString Checksum(const FString& Text){FTCHARToUTF8 Bytes(*Text);uint8 Hash[20];FSHA1::HashBuffer(Bytes.Get(),Bytes.Length(),Hash);return BytesToHex(Hash,20).ToLower();}
}
FString UMaiRulesSubsystem::SlotPath(const FString& Slot)const{if(Slot.IsEmpty()||Slot.Len()>64)return {};for(TCHAR C:Slot)if(!FChar::IsAlnum(C)&&C!=TEXT('_')&&C!=TEXT('-'))return {};FString Namespace;
#if !UE_BUILD_SHIPPING
FParse::Value(FCommandLine::Get(),TEXT("MaiCaptureSession="),Namespace);for(TCHAR Ch:Namespace)if(!FChar::IsAlnum(Ch)&&Ch!=TEXT('-')&&Ch!=TEXT('_'))return {};if(Namespace.Len()>80)return {};
#endif
return FPaths::ProjectSavedDir()/TEXT("NativeSaves")/(Namespace.IsEmpty()?TEXT("Player"):FString(TEXT("Verification"))/Namespace)/(Slot+TEXT(".mai.json"));}
TSharedPtr<FJsonObject> UMaiRulesSubsystem::Capture(){
    if(!IsReady()||!Company||!Company->Campaign())return nullptr;TSharedPtr<FJsonObject> Rules;
    auto R=Request(TEXT("save"));R->SetStringField(TEXT("savedAt"),FDateTime::UtcNow().ToIso8601());if(!Invoke(R,Rules))return nullptr;
    auto C=MakeShared<FJsonObject>();C->SetNumberField(TEXT("nativeFileVersion"),1);C->SetObjectField(TEXT("rules"),Rules);C->SetStringField(TEXT("campaign"),UTF8_TO_TCHAR(Company->Campaign()->Save().c_str()));
    auto Links=MakeShared<FJsonObject>();for(const auto& P:ReviewLinks)Links->SetNumberField(P.Key,double(P.Value));C->SetObjectField(TEXT("reviewLinks"),Links);return C;
}
bool UMaiRulesSubsystem::ApplyCapture(const TSharedPtr<FJsonObject>& Payload,bool ValidateOnly){
    if(!Payload||Number(Payload,TEXT("nativeFileVersion"))!=1||!Company||!Company->Campaign()){LastError=TEXT("Неподдерживаемый формат сохранения.");return false;}
    const auto Rules=Object(Payload,TEXT("rules")),Links=Object(Payload,TEXT("reviewLinks"));if(!Rules||!Links||Links->Values.Num()>512){LastError=TEXT("Повреждена история проверки данных.");return false;}
    auto Validate=Request(TEXT("validate"));Validate->SetObjectField(TEXT("payload"),Rules);TSharedPtr<FJsonObject> Out;if(!Invoke(Validate,Out))return false;
    mai::Campaign Candidate=*Company->Campaign();const FString Encoded=String(Payload,TEXT("campaign"));const auto Result=Candidate.Load(TCHAR_TO_UTF8(*Encoded));
    if(!Result.ok){LastError=UTF8_TO_TCHAR(Result.message.c_str());return false;}
    TMap<FString,int64> Parsed;TSet<int64> Unique;
    for(const auto& P:Links->Values){const FString LinkKey(*P.Key);double NumberValue=0;if(LinkKey.Len()>64||!LinkKey.StartsWith(TEXT("model-"))||!LinkKey.Contains(TEXT(":"))||!P.Value->TryGetNumber(NumberValue)||!FMath::IsFinite(NumberValue)||NumberValue<1||NumberValue>1000000000||NumberValue!=FMath::FloorToDouble(NumberValue)){LastError=TEXT("Некорректная ссылка на проверку данных.");return false;}
        const int64 Id=int64(NumberValue);if(!Candidate.Batch(Id)||Unique.Contains(Id)){LastError=TEXT("Потеряна или продублирована история партии данных.");return false;}Unique.Add(Id);Parsed.Add(LinkKey,Id);
    }
    const auto Canonical=Object(Object(Rules,TEXT("browser")),TEXT("game")),Ledger=Object(Canonical,TEXT("company"));
    if(!Ledger){LastError=TEXT("Нет канонического состояния компании.");return false;}
    // Both halves must describe the same transaction. A valid but unrelated campaign is not an acceptable save.
    const auto& C=Candidate.Core().View();
    if(Candidate.View().prologueStep==3&&(FMath::Abs(Number(Ledger,TEXT("cash"))-double(C.cash)/mai::Unit)>0.000002||FMath::Abs(Number(Ledger,TEXT("elapsedGameHours"))-double(C.now)/mai::Hour)>0.00000004)){LastError=TEXT("Части сохранения относятся к разным состояниям компании.");return false;}
    for(const auto& M:Array(Canonical,TEXT("models")))for(const auto& L:Array(Object(M->AsObject(),TEXT("state")),TEXT("queue"))){const FString Key=String(M->AsObject(),TEXT("id"))+TEXT(":")+FString::Printf(TEXT("%lld"),int64(Number(L->AsObject(),TEXT("id"))));if(!Parsed.Contains(Key)){LastError=TEXT("У партии данных нет истории проверки.");return false;}}
    if(ValidateOnly)return true;
    auto Load=Request(TEXT("load"));Load->SetObjectField(TEXT("payload"),Rules);if(!Invoke(Load,Out))return false;
    const FTCHARToUTF8 Bytes(*Encoded);TArray<uint8> Data;Data.Append(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    if(!Company->LoadPayload(Data).bSuccess){LastError=TEXT("Не удалось восстановить кампанию.");return false;}
    ReviewLinks=MoveTemp(Parsed);FrameCarry=0;AutoSaveCarry=0;bReviewOpen=false;return SyncReviews();
}
bool UMaiRulesSubsystem::SaveSlot(const FString& Slot){
    const FString Path=SlotPath(Slot);if(Path.IsEmpty()){LastError=TEXT("Некорректное имя сохранения.");return false;}
    if(bLoadingTransaction||!IsReady()||String(State,TEXT("phase"))!=TEXT("playing")){LastError=TEXT("Сейчас нельзя сохранить партию.");return false;}
    auto Payload=Capture();if(!Payload||!ApplyCapture(Payload,true))return false;
    const FString Text=Encode(Payload);auto Envelope=MakeShared<FJsonObject>();Envelope->SetStringField(TEXT("checksumSHA1"),Checksum(Text));Envelope->SetStringField(TEXT("payload"),Text);
    const FTCHARToUTF8 Bytes(*Encode(Envelope)),Filename(*Path);std::string Error;
    if(!mai::WriteDurableFile(std::string(Filename.Get(),Filename.Length()),std::string(Bytes.Get(),Bytes.Length()),Error)){LastError=UTF8_TO_TCHAR(Error.c_str());return false;}bHasSave=true;return true;
}
bool UMaiRulesSubsystem::LoadSlot(const FString& Slot){
    FString Path=SlotPath(Slot);if(Slot==TEXT("campaign")&&!FPaths::FileExists(Path)){const FString Auto=SlotPath(TEXT("autosave"));if(FPaths::FileExists(Auto))Path=Auto;}
    if(Path.IsEmpty()||bLoadingTransaction||(Company&&Company->Campaign()&&Company->Campaign()->View().loading.phase==mai::LoadPhase::Loading)){LastError=TEXT("Загрузка сейчас недоступна.");return false;}
    std::string Text,Error;const FTCHARToUTF8 Filename(*Path);if(!mai::ReadBoundedFile(std::string(Filename.Get(),Filename.Length()),Text,Error)){LastError=UTF8_TO_TCHAR(Error.c_str());return false;}
    const auto Envelope=Decode(UTF8_TO_TCHAR(Text.c_str()));const FString Body=String(Envelope,TEXT("payload"));
    if(!Envelope||Body.IsEmpty()||String(Envelope,TEXT("checksumSHA1"))!=Checksum(Body)){LastError=TEXT("Сохранение повреждено. Исходный файл и текущая партия сохранены.");return false;}
    const auto Payload=Decode(Body);if(!ApplyCapture(Payload,true))return false;RestoreRollback=Capture();if(!RestoreRollback)return false;
    if(auto* Loader=GetGameInstance()->GetSubsystem<UMaiLoadingSubsystem>())Loader->Invalidate();
    if(!ApplyCapture(Payload)){const FString Why=LastError;ApplyCapture(RestoreRollback);RestoreRollback.Reset();LastError=Why;return false;}
    auto* Campaign=Company->Campaign();bLoadingTransaction=true;
    if(Campaign->Core().View().ended){bLoadingTransaction=false;RestoreRollback.Reset();return true;}
    const auto Interior=Campaign->View().interior;const auto Destination=Interior.empty()?mai::Screen::CityMap:mai::Screen::Gameplay;
    if(!Company->CampaignTransact([&](mai::Campaign& C){return C.BeginLoad(Destination,Interior);}).bSuccess){bLoadingTransaction=false;const auto Previous=RestoreRollback;RestoreRollback.Reset();ApplyCapture(Previous);LastError=TEXT("Не удалось начать загрузку. Предыдущая партия восстановлена.");return false;}return true;
}
void UMaiRulesSubsystem::CheckRestore(){
    if(!bLoadingTransaction||!Company||!Company->Campaign())return;const auto& S=Company->Campaign()->View();
    if(S.loading.phase==mai::LoadPhase::Loading&&!bRestoreCancelled)return;
    if(S.loading.phase==mai::LoadPhase::Ready&&!bRestoreCancelled){bLoadingTransaction=false;RestoreRollback.Reset();SyncReviews();return;}
    const FString Reason=S.loading.phase==mai::LoadPhase::Failed?UTF8_TO_TCHAR(S.loading.error.c_str()):TEXT("Загрузка отменена.");
    if(auto* Loader=GetGameInstance()->GetSubsystem<UMaiLoadingSubsystem>())Loader->Invalidate();
    bLoadingTransaction=false;bRestoreCancelled=false;if(RestoreRollback){const auto Old=RestoreRollback;RestoreRollback.Reset();ApplyCapture(Old);
        if(auto* PC=Cast<AMaiPlayerController>(UGameplayStatics::GetPlayerController(GetGameInstance(),0))){FString SceneError;PC->PrepareCampaignScene(UTF8_TO_TCHAR(Company->Campaign()->View().interior.c_str()),!Company->Campaign()->CanPlay(),SceneError);}}
    LastError=Reason+TEXT(" Предыдущая партия восстановлена; файл сохранения не изменён.");
}
