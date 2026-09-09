#include "Verification/MaiCaptureSubsystem.h"
#include "Rules/MaiRulesSubsystem.h"
#include "UI/MaiNativeWidget.h"
#include "World/MaiPlayerController.h"
#include "World/MaiCameraPawn.h"
#include "World/MaiCityLighting.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystems/SubsystemCollection.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif
namespace {
FString Encode(const TSharedRef<FJsonObject>& O){FString S;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&S));return S;}
bool Equal(const TSharedPtr<FJsonValue>& A,const TSharedPtr<FJsonValue>& B){
    if(!A||!B||A->Type!=B->Type)return false;
    switch(A->Type){
    case EJson::Null:return true;
    case EJson::String:return A->AsString()==B->AsString();
    case EJson::Boolean:return A->AsBool()==B->AsBool();
    case EJson::Number:return FMath::Abs(A->AsNumber()-B->AsNumber())<=1e-9;
    case EJson::Array:{const auto& X=A->AsArray();const auto& Y=B->AsArray();if(X.Num()!=Y.Num())return false;for(int I=0;I<X.Num();++I)if(!Equal(X[I],Y[I]))return false;return true;}
    case EJson::Object:{const auto X=A->AsObject(),Y=B->AsObject();if(X->Values.Num()!=Y->Values.Num())return false;for(const auto& P:X->Values){const auto* V=Y->Values.Find(P.Key);if(!V||!Equal(P.Value,*V))return false;}return true;}
    default:return false;
    }
}
}
void UMaiCaptureSubsystem::Initialize(FSubsystemCollectionBase& C){Super::Initialize(C);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("MaiCaptureManual")))return;
    if(!FParse::Value(FCommandLine::Get(),TEXT("MaiCaptureSession="),Session))return;
    if(Session.IsEmpty()||Session.Len()>80)return;for(TCHAR Ch:Session)if(!FChar::IsAlnum(Ch)&&Ch!=TEXT('-')&&Ch!=TEXT('_'))return;
    C.InitializeDependency<UMaiRulesSubsystem>();Rules=GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>();
    Resume=FParse::Param(FCommandLine::Get(),TEXT("MaiCaptureResume"));
    Directory=FPaths::ProjectSavedDir()/TEXT("Verification")/Session/(Resume?TEXT("resume"):TEXT("capture"));
    if(IFileManager::Get().DirectoryExists(*Directory)){UE_LOG(LogTemp,Error,TEXT("Capture directory already exists; never overwrite evidence"));return;}
    IFileManager::Get().MakeDirectory(*Directory,true);Active=true;Started=FPlatformTime::Seconds();Next=Started+5;
    if(Resume&&Rules)Rules->SetForeground(false); // stop clock before restoring, not a time-accelerated test
#endif
}
UWorld* UMaiCaptureSubsystem::GetTickableGameObjectWorld()const{return GetGameInstance()?GetGameInstance()->GetWorld():nullptr;}
UMaiNativeWidget* UMaiCaptureSubsystem::UI()const{auto* PC=Cast<AMaiPlayerController>(UGameplayStatics::GetPlayerController(GetGameInstance(),0));return PC?PC->NativeUI():nullptr;}
bool UMaiCaptureSubsystem::Click(const FString& Id){auto* W=UI();if(!W||!W->ActionIds().Contains(Id))return false;W->Dispatch(Id);Next=FPlatformTime::Seconds()+.7;return true;}
void UMaiCaptureSubsystem::Capture(const FString& Name,int32 NextStage){
#if WITH_EDITOR
    if(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling()){Next=FPlatformTime::Seconds()+1;return;}
#endif
    if(RecentFrameMs.Num()>30){
        auto Sorted=RecentFrameMs;Sorted.Sort();double Sum=0;for(double Ms:Sorted)Sum+=Ms;
        auto Sample=MakeShared<FJsonObject>();Sample->SetStringField(TEXT("screen"),Name);
        Sample->SetNumberField(TEXT("sampleCount"),Sorted.Num());Sample->SetNumberField(TEXT("meanFrameMs"),Sum/Sorted.Num());
        Sample->SetNumberField(TEXT("p95FrameMs"),Sorted[FMath::Min(Sorted.Num()-1,FMath::FloorToInt(Sorted.Num()*.95))]);
        Sample->SetNumberField(TEXT("meanFPS"),1000./(Sum/Sorted.Num()));PerformanceSamples.Add(MakeShared<FJsonValueObject>(Sample));
    }
    FString Error;if(!UI()||!UI()->ValidateViewport(Error)){Finish(TEXT("Viewport layout: ")+Error);return;}
    PendingShot=Directory/(Name+TEXT(".png"));AfterShot=NextStage;ShotStarted=FPlatformTime::Seconds();
    FScreenshotRequest::RequestScreenshot(PendingShot,true,false); // includes real Slate/UMG, never Blender imagery
}
void UMaiCaptureSubsystem::Finish(const FString& Error){
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("scope"),TEXT("Native UMG action smoke and GPU captures; not a full gameplay or human playthrough"));
    Report->SetBoolField(TEXT("humanPlaytest"),false);Report->SetBoolField(TEXT("fullFlowVerified"),false);
    Report->SetStringField(TEXT("performanceScope"),TEXT("Recent game frame deltas per screen; includes transition frames, not isolated GPU timings or a sustained benchmark"));
    Report->SetArrayField(TEXT("performance"),PerformanceSamples);
    Report->SetStringField(TEXT("inputRoute"),TEXT("UMG semantic action dispatch; OS pointer hit testing remains manual"));
    Report->SetBoolField(TEXT("completed"),Error.IsEmpty());Report->SetBoolField(TEXT("processRestartLoadVerified"),Resume&&Error.IsEmpty());Report->SetStringField(TEXT("error"),Error);
    TArray<TSharedPtr<FJsonValue>> Paths;for(const auto& P:Captures)Paths.Add(MakeShared<FJsonValueString>(P));Report->SetArrayField(TEXT("captures"),Paths);
    FFileHelper::SaveStringToFile(Encode(Report),*(Directory/TEXT("result.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    Active=false;if(Rules)Rules->SetForeground(false);
    UKismetSystemLibrary::QuitGame(GetGameInstance(),UGameplayStatics::GetPlayerController(GetGameInstance(),0),EQuitPreference::Quit,false);
}
void UMaiCaptureSubsystem::Tick(float Delta){if(!Active)return;const double Now=FPlatformTime::Seconds();
    if(Delta>0&&FMath::IsFinite(Delta)){RecentFrameMs.Add(Delta*1000.);if(RecentFrameMs.Num()>300)RecentFrameMs.RemoveAt(0);}
    if(Now-Started>600){Finish(TEXT("Capture timed out; no PASS was recorded"));return;}
    if(!PendingShot.IsEmpty()){
        if(FPaths::FileExists(PendingShot)){TArray<uint8> Png;FFileHelper::LoadFileToArray(Png,*PendingShot);const uint8 Magic[8]={137,80,78,71,13,10,26,10};
            if(Png.Num()<24||FMemory::Memcmp(Png.GetData(),Magic,8)!=0){Finish(TEXT("Screenshot is not a PNG"));return;}
            const auto Read=[&](int Offset){return (uint32(Png[Offset])<<24)|(uint32(Png[Offset+1])<<16)|(uint32(Png[Offset+2])<<8)|Png[Offset+3];};
            auto* PC=UGameplayStatics::GetPlayerController(GetGameInstance(),0);int32 X=0,Y=0;PC->GetViewportSize(X,Y);
            if(Read(16)!=uint32(X)||Read(20)!=uint32(Y)){Finish(TEXT("Screenshot dimensions do not match the actual viewport"));return;}
            Captures.Add(FPaths::ConvertRelativePathToFull(PendingShot));PendingShot.Reset();Stage=AfterShot;Next=Now+.5;
        }else if(Now-ShotStarted>45)Finish(TEXT("No GPU screenshot was produced"));return;
    }
    if(Now<Next||!Rules||!Rules->IsReady()||!UI())return;
    if(Resume){
        if(Stage==0){if(Click(TEXT("continue")))Stage=1;return;}
        if(Stage==1){auto State=Rules->CanonicalState();if(!State||!UI()->ActionIds().Contains(TEXT("nav-training")))return;
            FString Text;TSharedPtr<FJsonObject> Saved;if(!FFileHelper::LoadFileToString(Text,*(FPaths::GetPath(Directory)/TEXT("expected-company.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Saved)){Finish(TEXT("Restart oracle unavailable"));return;}
            const auto* Expected=Saved->Values.Find(TEXT("company"));const auto* Actual=State->Values.Find(TEXT("company"));if(!Expected||!Actual||!Equal(*Expected,*Actual)){Finish(TEXT("Saved company changed across actual process restart"));return;}
            Capture(TEXT("restored-company"),2);return;
        }Finish({});return;
    }
    switch(Stage){
    case 0:if(UI()->ActionIds().Contains(TEXT("new-game")))Capture(TEXT("01-menu"),1);break;
    case 1:if(Click(TEXT("new-game")))Stage=2;break;
    case 2:UI()->Dispatch(TEXT("name"),MakeShared<FJsonValueString>(TEXT("Capture Company")));Capture(TEXT("02-setup"),3);break;
    case 3:if(Click(TEXT("setup-next")))Stage=4;break;
    case 4:if(UI()->ActionIds().Contains(TEXT("Normal-choose")))Capture(TEXT("03-difficulty"),5);break;
    case 5:if(Click(TEXT("Normal-choose")))Stage=6;break;
    case 6:if(UI()->ActionIds().Contains(TEXT("prologue-create")))Capture(TEXT("04-prologue"),7);break;
    case 7:if(Click(TEXT("prologue-create")))Stage=8;break;
    case 8:if(Click(TEXT("prologue-budget-next")))Stage=9;break;
    case 9:if(Click(TEXT("prologue-play")))Stage=10;break;
    case 10:if(Click(TEXT("friendly")))Stage=11;break;
    case 11:if(UI()->ActionIds().Contains(TEXT("buy-location")))Capture(TEXT("05-city"),30);break;
    case 30:{auto* Camera=Cast<AMaiCameraPawn>(UGameplayStatics::GetPlayerPawn(GetGameInstance(),0));if(!Camera){Finish(TEXT("City camera missing"));break;}
        const FVector Position=Camera->GetActorLocation();const FVector Forward=Camera->GetActorForwardVector();
        const FVector Target=Position+Forward*(-Position.Z/Forward.Z);
        Camera->SetActorLocation(Target+(Position-Target)*.12);Stage=31;Next=Now+4;break;}
    case 31:Capture(TEXT("05b-city-close-day"),32);break;
    case 32:for(TActorIterator<AMaiCityLighting> It(GetTickableGameObjectWorld());It;++It){It->SetActorTickEnabled(false);It->ApplyPreview(true);}Stage=33;Next=Now+5;break;
    case 33:Capture(TEXT("05c-city-close-night"),34);break;
    case 34:for(TActorIterator<AMaiCityLighting> It(GetTickableGameObjectWorld());It;++It){It->ApplyPreview(false);It->SetActorTickEnabled(true);}
        if(auto* Camera=Cast<AMaiCameraPawn>(UGameplayStatics::GetPlayerPawn(GetGameInstance(),0)))Camera->ResetOverview();Stage=12;Next=Now+2;break;
    case 12:if(Click(TEXT("buy-location")))Stage=13;break;
    case 13:if(Click(TEXT("open-location")))Stage=43;break;
    case 43:if(Click(TEXT("walk-in")))Stage=40;break;
    case 40:if(UI()->ActionIds().Contains(TEXT("leave-interior")))Capture(TEXT("05d-walkable-garage"),41);break;
    case 41:if(Click(TEXT("leave-interior")))Stage=42;break;
    case 42:if(UI()->ActionIds().Contains(TEXT("open-location")))Capture(TEXT("05e-city-after-interior"),44);break;
    case 44:if(Click(TEXT("open-location")))Stage=14;break;
    case 14:Capture(TEXT("06-equipment"),15);break;
    case 15:if(Click(TEXT("procurement")))Stage=16;break;
    case 16:Capture(TEXT("07-procurement"),17);break;
    case 17:if(Click(TEXT("place-order")))Stage=18;break;
    case 18:Capture(TEXT("08-ordered"),19);break;
    case 19:if(Click(TEXT("close-dialog")))Stage=20;break;
    case 20:if(Click(TEXT("nav-training")))Stage=21;break;
    case 21:Capture(TEXT("09-training"),45);break;
    case 45:if(UI()->RevealContentNode(TEXT("training-run"))){Stage=46;Next=Now+1;}else Finish(TEXT("Training controls not found"));break;
    case 46:Capture(TEXT("09b-training-controls"),22);break;
    case 22:{if(!Rules->SaveSlot()){Finish(TEXT("Real save failed: ")+Rules->LastError);break;}
        auto Expected=MakeShared<FJsonObject>();Expected->SetObjectField(TEXT("company"),Rules->CanonicalState()->GetObjectField(TEXT("company")));
        if(!FFileHelper::SaveStringToFile(Encode(Expected),*(FPaths::GetPath(Directory)/TEXT("expected-company.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)){Finish(TEXT("Cannot retain restart oracle"));break;}
        Finish({});break;}
    default:Finish(TEXT("Invalid capture stage"));break;
    }
}
