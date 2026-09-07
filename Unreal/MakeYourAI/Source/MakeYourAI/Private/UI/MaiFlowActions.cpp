#include "UI/MaiFlowWidget.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Campaign/MaiLoadingSubsystem.h"
#include "World/MaiPlayerController.h"
#include "Persistence/MaiSaveSubsystem.h"
#include "Components/EditableTextBox.h"
#include "Engine/GameInstance.h"
#include "Misc/LexFromString.h"
void UMaiFlowWidget::Command(const FString& Action){
    if(!Company || !Company->CampaignDomain()) return;
    auto* PC=Cast<AMaiPlayerController>(GetOwningPlayer());auto* Saves=GetGameInstance()->GetSubsystem<UMaiSaveSubsystem>();
    FMaiActionResult R;R.bSuccess=true;R.Message=FText::FromString(TEXT("Completed"));
    const auto Run=[&](TFunctionRef<mai::Result(mai::Campaign&)> Function){R=Company->CampaignTransact(Function);};
    if(Action==TEXT("retry")){GetGameInstance()->GetSubsystem<UMaiLoadingSubsystem>()->Retry();}
    else if(Action==TEXT("new")){Run([](mai::Campaign& G){return G.BeginNewGame();});SelectedBatch=0;}
    else if(Action==TEXT("difficulty")) Run([](mai::Campaign& G){return G.ShowDifficulty();});
    else if(Action.StartsWith(TEXT("difficulty:"))) Run([&](mai::Campaign& G){return G.ChooseDifficulty(TCHAR_TO_UTF8(*Action.Mid(11)));});
    else if(Action==TEXT("prologue-name")){const FString Name=CompanyName?CompanyName->GetText().ToString():FString();Run([&](mai::Campaign& G){return G.PrologueAction(TCHAR_TO_UTF8(*Name));});}
    else if(Action==TEXT("prologue-budget")) Run([](mai::Campaign& G){return G.PrologueAction("budget");});
    else if(Action==TEXT("prologue-task")) Run([](mai::Campaign& G){return G.PrologueAction("accept-task");});
    else if(Action==TEXT("resume")) Run([](mai::Campaign& G){return G.BeginLoad(G.View().interior.empty()?mai::Screen::CityMap:mai::Screen::Gameplay,G.View().interior);});
    else if(Action==TEXT("menu")) Run([](mai::Campaign& G){return G.ShowScreen(mai::Screen::MainMenu);});
    else if(Action==TEXT("training")) Run([](mai::Campaign& G){return G.ShowScreen(mai::Screen::Training);});
    else if(Action==TEXT("operations") && PC) PC->ToggleOperations();
    else if(Action==TEXT("city")) Run([](mai::Campaign& G){return G.BeginLoad(mai::Screen::CityMap);});
    else if(Action==TEXT("back")) Run([](mai::Campaign& G){return G.View().interior.empty()?G.BeginLoad(mai::Screen::CityMap):G.ShowScreen(mai::Screen::Gameplay);});
    else if(Action==TEXT("pause")) R=Company->SetPaused(!Company->Domain()->View().paused);
    else if(Action==TEXT("save")) R=Saves->SaveCompany(TEXT("MakeYourAI_01"));
    else if(Action==TEXT("load")) R=Saves->LoadCompany(TEXT("MakeYourAI_01"));
    else if(Action==TEXT("return-save")){
        if(Saves->CanReturnToSave(TEXT("MakeYourAI_01"))) R=Saves->LoadCompany(TEXT("MakeYourAI_01"));
        else R=FMaiActionResult::From(mai::Result::Error("This ending or save does not permit returning"));
    }
    else if(Action.StartsWith(TEXT("buy-data:"))){Run([&](mai::Campaign& G){return G.BuyDataset(TCHAR_TO_UTF8(*Action.Mid(9)));});if(R.bSuccess) SelectedBatch=Company->CampaignDomain()->View().inventory.sequence;}
    else if(Action.StartsWith(TEXT("select:"))){int64 Id=0;if(LexTryParseString(Id,*Action.Mid(7)) && Company->CampaignDomain()->Batch(Id)) SelectedBatch=Id;else R=FMaiActionResult::From(mai::Result::Error("Dataset no longer exists"));}
    else if(Action.StartsWith(TEXT("review:"))){
        const FString Method=Action.Mid(7);
        if(Method==TEXT("manual") && (!PC || !PC->AtReviewDesk())) R=FMaiActionResult::From(mai::Result::Error("Approach the review desk inside Garage"));
        else Run([&](mai::Campaign& G){return G.StartReview(SelectedBatch,Method==TEXT("manual")?mai::ReviewMethod::Manual:Method==TEXT("human")?mai::ReviewMethod::Human:Method==TEXT("ai")?mai::ReviewMethod::AI:mai::ReviewMethod::None);});
    }else if(Action.StartsWith(TEXT("choice:"))){
        TArray<FString> Parts;Action.ParseIntoArray(Parts,TEXT(":"));int64 Id=0;int32 Side=-1;
        if(Parts.Num()==3 && LexTryParseString(Id,*Parts[1]) && LexTryParseString(Side,*Parts[2])) Run([&](mai::Campaign& G){return G.ChooseReview(Id,Side);});
        else R=FMaiActionResult::From(mai::Result::Error("Invalid review decision"));
    }
    else if(Action==TEXT("hire")) Run([](mai::Campaign& G){return G.HireSpecialist();});
    else if(Action==TEXT("ai-create")) Run([](mai::Campaign& G){return G.CreateAIReviewer();});
    else if(Action==TEXT("ai-upgrade")) Run([](mai::Campaign& G){return G.ImproveAIReviewer();});
    else if(Action==TEXT("overwork")) Run([](mai::Campaign& G){return G.SetOverwork(!G.View().overwork);});
    else if(Action==TEXT("train")) Run([&](mai::Campaign& G){return G.StartTraining(SelectedBatch);});
    else if(Action==TEXT("job-pause") || Action==TEXT("job-resume")) Run([&](mai::Campaign& G){return G.PauseTraining(Action==TEXT("job-pause"));});
    else if(Action==TEXT("evaluate")) Run([](mai::Campaign& G){return G.EvaluateEnding();});
    else if(Action==TEXT("open-model")) Run([](mai::Campaign& G){return G.ChooseEnding(mai::EndingKind::OpenModel);});
    else if(Action==TEXT("sale")) Run([](mai::Campaign& G){return G.ChooseEnding(mai::EndingKind::Acquisition);});
    else if(Action==TEXT("decline")) Run([](mai::Campaign& G){return G.DeclineSale();});
    else if(Action==TEXT("remediate")) Run([](mai::Campaign& G){return G.Remediate();});
    else if(Action==TEXT("ignore")) Run([](mai::Campaign& G){return G.IgnoreWarning();});
    else if(Action==TEXT("borrow")) Run([](mai::Campaign& G){return G.Borrow(mai::Dollars(1000));});
    else if(Action==TEXT("repay")) Run([](mai::Campaign& G){return G.Repay(mai::Dollars(1000));});
    else R=FMaiActionResult::From(mai::Result::Error("Unknown campaign command"));
    Message=R.Message.ToString();Signature.Empty();Refresh();
}
