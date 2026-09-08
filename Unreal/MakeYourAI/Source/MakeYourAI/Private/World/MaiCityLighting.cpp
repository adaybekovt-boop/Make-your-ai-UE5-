#include "World/MaiCityLighting.h"
#include "Rules/MaiRulesSubsystem.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/GameInstance.h"
#include "World/MaiCityBatch.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Engine/PostProcessVolume.h"
AMaiCityLighting::AMaiCityLighting(){PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=1.f;}
void AMaiCityLighting::BeginPlay(){
    Super::BeginPlay();
    TMap<UMaterialInterface*,UMaterialInstanceDynamic*> Shared;
    for(TActorIterator<AMaiCityBatch> It(GetWorld());It;++It){auto* Mesh=It->Instances.Get();if(!Mesh)continue;
        for(int32 I=0;I<Mesh->GetNumMaterials();++I){auto* Parent=Mesh->GetMaterial(I);if(!Parent)continue;
            if(auto** Existing=Shared.Find(Parent)){Mesh->SetMaterial(I,*Existing);continue;}
            float Strength=0;if(!Parent->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissionStrength")),Strength)||Strength<=0)continue;
            auto* Dynamic=UMaterialInstanceDynamic::Create(Parent,this);Shared.Add(Parent,Dynamic);
            WindowMaterials.Add(Dynamic);WindowEmissionStrengths.Add(Strength);Mesh->SetMaterial(I,Dynamic);
        }
    }
    ApplyHour(LastHour<0?8.:LastHour);
}
void AMaiCityLighting::ApplyHour(double Hour){
    const double Elevation=FMath::Sin((Hour-6.)/24.*2.*PI);
    const float Day=FMath::SmoothStep(-.08f,.35f,float(Elevation));
    // Match exposure to the light cycle, not to how much bright water/sky the
    // player happens to frame. Equal EV bounds remove long adaptation blackouts.
    for(TActorIterator<APostProcessVolume> It(GetWorld());It;++It){if(!It->ActorHasTag(TEXT("MAI_GeneratedCity")))continue;
        auto& Settings=It->Settings;
        Settings.bOverride_AutoExposureMinBrightness=true;Settings.bOverride_AutoExposureMaxBrightness=true;
        Settings.AutoExposureMinBrightness=Settings.AutoExposureMaxBrightness=FMath::Lerp(-3.f,9.f,Day);
        Settings.bOverride_BloomIntensity=true;Settings.BloomIntensity=.12f;
        Settings.bOverride_LensFlareIntensity=true;Settings.LensFlareIntensity=0.f;
    }
    for(int32 I=0;I<WindowMaterials.Num();++I)WindowMaterials[I]->SetScalarParameterValue(TEXT("EmissionStrength"),WindowEmissionStrengths[I]*(1.f-Day));
    if(Sun){auto* L=Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
        L->SetIntensity(FMath::Lerp(.4f,12000.f,Day));
        L->SetLightColor(FMath::Lerp(FLinearColor(.65f,.75f,1.f),FLinearColor(1.f,.96f,.88f),Day));
        L->SetLightSourceAngle(3.f);
        Sun->SetActorRotation(FRotator(-FMath::Max(12.,60.*FMath::Abs(Elevation)),-35.+(Hour-8.)*8.,0));
    }
    if(Sky)Sky->GetLightComponent()->SetIntensity(FMath::Lerp(.35f,1.8f,Day));
    PreviewNight=Day<.1f;LastHour=Hour;
}
void AMaiCityLighting::ApplyPreview(bool Night){ApplyHour(Night?0.:8.);}
void AMaiCityLighting::Tick(float Delta){Super::Tick(Delta);if(!GetGameInstance())return;
    auto* Rules=GetGameInstance()->GetSubsystem<UMaiRulesSubsystem>();auto State=Rules?Rules->CanonicalState():nullptr;
    if(!State)return;const TSharedPtr<FJsonObject>* Company=nullptr;const TSharedPtr<FJsonObject>* Ledger=nullptr;
    if(State->TryGetObjectField(TEXT("company"),Company)&&(*Company)->TryGetObjectField(TEXT("company"),Ledger)){
        double Hours=0;if((*Ledger)->TryGetNumberField(TEXT("elapsedGameHours"),Hours)){const double Hour=FMath::Fmod(Hours+8,24);if(LastHour<0||FMath::Abs(Hour-LastHour)>.01)ApplyHour(Hour);}}
}
