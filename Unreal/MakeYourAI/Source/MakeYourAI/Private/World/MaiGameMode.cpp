#include "World/MaiGameMode.h"
#include "World/MaiCameraPawn.h"
#include "World/MaiPlayerController.h"
#include "World/MaiScaffoldWorld.h"
#include "EngineUtils.h"
#include "Engine/World.h"

AMaiGameMode::AMaiGameMode() { DefaultPawnClass = AMaiCameraPawn::StaticClass(); PlayerControllerClass = AMaiPlayerController::StaticClass(); }
void AMaiGameMode::BeginPlay() {
    Super::BeginPlay();
    if (!GetWorld()) return;
    for (TActorIterator<AMaiScaffoldWorld> It(GetWorld()); It; ++It) return;
    GetWorld()->SpawnActor<AMaiScaffoldWorld>();
}
