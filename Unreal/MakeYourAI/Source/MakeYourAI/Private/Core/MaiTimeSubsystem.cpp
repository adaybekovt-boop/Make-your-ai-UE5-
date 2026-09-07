#include "Core/MaiTimeSubsystem.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "Subsystems/SubsystemCollection.h"

void UMaiTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection); Collection.InitializeDependency<UMaiCompanySubsystem>();
    Company = GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UMaiTimeSubsystem::EnterBackground);
}
void UMaiTimeSubsystem::Deinitialize() {
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    Company = nullptr; Super::Deinitialize();
}
void UMaiTimeSubsystem::EnterBackground() { if (Company) Company->SetPaused(true); }
UWorld* UMaiTimeSubsystem::GetTickableGameObjectWorld() const { return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr; }
bool UMaiTimeSubsystem::IsTickable() const {
    const auto* World = GetTickableGameObjectWorld();
    return !IsTemplate() && Company && Company->IsReady() && World && World->IsGameWorld();
}
void UMaiTimeSubsystem::Tick(float DeltaTime) {
    if (!Company || !Company->Domain() || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0) return;
    if (SeenGeneration != Company->Generation()) { SeenGeneration = Company->Generation(); FractionalMicroseconds = 0; }
    if (Company->Domain()->View().paused || Company->Domain()->View().ended) return;
    // Only this subsystem advances the company. No global time dilation, no
    // second economy tick, no wall-clock catch-up when a save is loaded.
    const double Exact = static_cast<double>(DeltaTime) * 1000000.0 + FractionalMicroseconds;
    const int64 Whole = static_cast<int64>(Exact);
    FractionalMicroseconds = Exact - static_cast<double>(Whole);
    Company->Advance(Whole);
}
double UMaiTimeSubsystem::GetGameHours() const { return Company && Company->Domain() ? static_cast<double>(Company->Domain()->View().now) / mai::Hour : 0; }
