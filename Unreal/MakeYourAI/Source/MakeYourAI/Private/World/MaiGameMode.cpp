#include "World/MaiGameMode.h"
#include "World/MaiCameraPawn.h"
#include "World/MaiPlayerController.h"
#include "World/MaiScaffoldWorld.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

AMaiGameMode::AMaiGameMode() { DefaultPawnClass = AMaiCameraPawn::StaticClass(); PlayerControllerClass = AMaiPlayerController::StaticClass(); }
void AMaiGameMode::BeginPlay() {
    Super::BeginPlay();
    if(GEngine&&GEngine->GetGameUserSettings()){
        auto* Settings=GEngine->GetGameUserSettings();
        if(Settings->GetFrameRateLimit()<=0)Settings->SetFrameRateLimit(60);
        Settings->ApplyNonResolutionSettings();
    }
    // City content is loaded by MaiLoadingSubsystem. Never spawn a competing
    // graybox, duplicate lights or a second economy into an imported map.
}
