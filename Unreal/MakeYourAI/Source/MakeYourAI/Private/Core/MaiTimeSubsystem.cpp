#include "Core/MaiTimeSubsystem.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Rules/MaiRulesSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "Subsystems/SubsystemCollection.h"

void UMaiTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection); Collection.InitializeDependency<UMaiCompanySubsystem>();
    Collection.InitializeDependency<UMaiRulesSubsystem>();
    Rules = GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>();
    Company = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    ForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UMaiTimeSubsystem::EnterForeground);
    BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UMaiTimeSubsystem::EnterBackground);
}
void UMaiTimeSubsystem::Deinitialize() {
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
    Rules = nullptr; Company = nullptr; Super::Deinitialize();
}
void UMaiTimeSubsystem::EnterBackground() { if (Rules) Rules->SetForeground(false); }
void UMaiTimeSubsystem::EnterForeground() { if (Rules) Rules->SetForeground(true); }
UWorld* UMaiTimeSubsystem::GetTickableGameObjectWorld() const { return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr; }
bool UMaiTimeSubsystem::IsTickable() const {
    const auto* World = GetTickableGameObjectWorld();
    return !IsTemplate() && Company && Company->IsReady() && World && World->IsGameWorld();
}
void UMaiTimeSubsystem::Tick(float DeltaTime) {
    // Exactly one clock and ledger. Missing rules fail closed, never fall back to
    // the historical portable economy while the canonical state is active.
    if (Rules) Rules->AdvanceFrame(DeltaTime);
}
double UMaiTimeSubsystem::GetGameHours() const { return Company && Company->Domain() ? static_cast<double>(Company->Domain()->View().now) / mai::Hour : 0; }
