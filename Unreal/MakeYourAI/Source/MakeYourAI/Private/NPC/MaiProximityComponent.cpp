#include "NPC/MaiProximityComponent.h"
#include "Gameplay/MaiCompanySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UMaiProximityComponent::UMaiProximityComponent() { PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.TickInterval = 0.1f; }
void UMaiProximityComponent::BeginPlay() {
    Super::BeginPlay();
    if (!Settings) Settings = NewObject<UMaiProximityEventAsset>(this);
}
void UMaiProximityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) {
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (!GetWorld() || !GetWorld()->GetGameInstance() || !Settings || !GetOwner()) return;
    auto* Company = GetWorld()->GetGameInstance()->GetSubsystem<UMaiCompanySubsystem>();
    const auto* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!Company || !Company->Domain() || !Player) return;
    if (!FMath::IsFinite(Settings->RadiusCm) || Settings->RadiusCm <= 0 || Settings->StageGameMinutes < 1 || Settings->StageGameMinutes > 60 || Settings->CooldownGameMinutes < 2 * Settings->StageGameMinutes || Settings->CooldownGameMinutes > 1440) return;
    const bool bInside = FVector::DistSquared(Player->GetActorLocation(), GetOwner()->GetActorLocation()) <= FMath::Square(Settings->RadiusCm);
    Company->Proximity(bInside, static_cast<int64>(Settings->StageGameMinutes) * mai::Hour / 60, static_cast<int64>(Settings->CooldownGameMinutes) * mai::Hour / 60);
    const auto Next = static_cast<EMaiNpcState>(Company->Domain()->View().npc.mode);
    if (Next != State) {
        State = Next;
        CurrentLine = State == EMaiNpcState::PhoneCall ? Settings->PhoneLine : State == EMaiNpcState::React ? Settings->ReactionLine : FText::GetEmpty();
        OnStateChanged.Broadcast(State, CurrentLine); // Animation and audio connect here; never claimed present.
    }
}
