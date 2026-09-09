#pragma once
#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "MaiServerAmbience.generated.h"

// Original synthesized fan ambience, with no downloaded/licensed recording.
UCLASS(ClassGroup=Audio)
class MAKEYOURAI_API UMaiServerAmbience : public USynthComponent {
    GENERATED_BODY()
public:
    UMaiServerAmbience(const FObjectInitializer& Initializer);
    void SetServerLoad(float Load);
protected:
    virtual bool Init(int32& SampleRate) override;
    virtual int32 OnGenerateAudio(float* OutAudio,int32 NumSamples) override;
private:
    float LastRequested=-1,Target=0,Gain=0,Low=0,Slow=0;
    float Rate=48000;
    double Phase=0;
    uint32 NoiseState=0x73519a21;
};
