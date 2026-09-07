#include "Persistence/MaiSaveSubsystem.h"
#include "Persistence/MaiSaveGame.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Campaign/MaiLoadingSubsystem.h"

bool UMaiSaveSubsystem::IsSafeSlotName(const FString& Name) {
    if (Name.IsEmpty() || Name.Len() > 64) return false;
    for (TCHAR C : Name) if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '_' || C == '-')) return false;
    return true;
}
bool UMaiSaveSubsystem::HasSave(const FString& Name) const { return IsSafeSlotName(Name) && UGameplayStatics::DoesSaveGameExist(Name, 0); }
FMaiActionResult UMaiSaveSubsystem::SaveCompany(const FString& Name) {
    auto* Company = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if (!IsSafeSlotName(Name) || !Company || !Company->IsReady()) return FMaiActionResult::From(mai::Result::Error("Invalid save slot or company"));
    auto* Save = Cast<UMaiSaveGame>(UGameplayStatics::CreateSaveGameObject(UMaiSaveGame::StaticClass()));
    if (!Save) return FMaiActionResult::From(mai::Result::Error("Could not create USaveGame"));
    Save->DomainPayload = Company->SavePayload();
    const auto& State=Company->CampaignDomain()->View();
    Save->Difficulty=UTF8_TO_TCHAR(State.difficulty.c_str()); Save->CurrentScreen=static_cast<int32>(State.screen);
    Save->FirstScreen=static_cast<int32>(State.firstScreen); Save->LastScreen=static_cast<int32>(State.lastScreen);
    TArray<uint8> Encoded;
    if (!UGameplayStatics::SaveGameToMemory(Save, Encoded)) return FMaiActionResult::From(mai::Result::Error("USaveGame serialization failed"));
    auto* Verify = Cast<UMaiSaveGame>(UGameplayStatics::LoadGameFromMemory(Encoded));
    if (!Verify || Verify->FormatVersion != 2 || Verify->DomainPayload != Save->DomainPayload) return FMaiActionResult::From(mai::Result::Error("USaveGame round-trip check failed"));
    if (!UGameplayStatics::SaveGameToSlot(Save, Name, 0)) return FMaiActionResult::From(mai::Result::Error("SaveGameToSlot failed; check storage"));
    return FMaiActionResult::From(mai::Result::Success("Company saved: money, clock, RNG, orders, warehouse, servers and extension state"));
}
FMaiActionResult UMaiSaveSubsystem::LoadCompany(const FString& Name) {
    if (!IsSafeSlotName(Name)) return FMaiActionResult::From(mai::Result::Error("Invalid save slot"));
    auto* Company = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if (!Company || !Company->IsReady()) return FMaiActionResult::From(mai::Result::Error("Company unavailable"));
    auto* Save = Cast<UMaiSaveGame>(UGameplayStatics::LoadGameFromSlot(Name, 0));
    if (!Save || (Save->FormatVersion != 1 && Save->FormatVersion != 2)) return FMaiActionResult::From(mai::Result::Error("Missing, corrupt or unsupported UE save"));
    auto Candidate=*Company->CampaignDomain();
    if(Save->DomainPayload.IsEmpty() || Save->DomainPayload.Num()>8*1024*1024) return FMaiActionResult::From(mai::Result::Error("Invalid campaign payload length"));
    auto Parsed=Candidate.Load(std::string(reinterpret_cast<const char*>(Save->DomainPayload.GetData()),Save->DomainPayload.Num()));
    if(!Parsed.ok) return FMaiActionResult::From(Parsed);
    const auto& V=Candidate.View();
    if(Save->FormatVersion==2 && (Save->Difficulty!=UTF8_TO_TCHAR(V.difficulty.c_str()) || Save->CurrentScreen!=static_cast<int32>(V.screen) || Save->FirstScreen!=static_cast<int32>(V.firstScreen) || Save->LastScreen!=static_cast<int32>(V.lastScreen))) return FMaiActionResult::From(mai::Result::Error("Campaign metadata/payload mismatch"));
    auto Result = Company->LoadPayload(Save->DomainPayload);
    if(Result.bSuccess) {
        GetGameInstance()->GetSubsystem<UMaiLoadingSubsystem>()->Invalidate();
        if(Candidate.CanPlay()) Company->CampaignTransact([&](mai::Campaign& C){return C.BeginLoad(V.screen,V.interior);});
    }
    if (Result.bSuccess) Result.Message = FText::FromString(TEXT("Company loaded without offline time progression"));
    return Result;
}

bool UMaiSaveSubsystem::CanReturnToSave(const FString& Name) const {
    if(!IsSafeSlotName(Name)) return false;
    auto* Company=GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    if(!Company || !Company->CampaignDomain() || !Company->CampaignDomain()->View().ending.allowReturnToSave) return false;
    auto* Save=Cast<UMaiSaveGame>(UGameplayStatics::LoadGameFromSlot(Name,0));
    if(!Save || (Save->FormatVersion!=1 && Save->FormatVersion!=2) || Save->DomainPayload.IsEmpty() || Save->DomainPayload.Num()>8*1024*1024) return false;
    auto Candidate=*Company->CampaignDomain();
    return Candidate.Load(std::string(reinterpret_cast<const char*>(Save->DomainPayload.GetData()),Save->DomainPayload.Num())).ok && Candidate.CanPlay() && !Candidate.Core().View().ended;
}
