#include "World/MaiServerAmbience.h"

UMaiServerAmbience::UMaiServerAmbience(const FObjectInitializer& Initializer):Super(Initializer){
    NumChannels=1;bAutoActivate=false;bAllowSpatialization=false;
}
bool UMaiServerAmbience::Init(int32& SampleRate){Rate=FMath::Max(8000,SampleRate);return true;}
void UMaiServerAmbience::SetServerLoad(float Load){
    Load=FMath::Clamp(Load,0.f,1.f);
    if(FMath::IsNearlyEqual(LastRequested,Load,.01f))return;
    LastRequested=Load;
    SynthCommand([this,Load](){Target=Load;});
}
int32 UMaiServerAmbience::OnGenerateAudio(float* OutAudio,int32 NumSamples){
    const float Smooth=1.f-FMath::Exp(-1.f/(Rate*.25f));
    const float Filter=1.f-FMath::Exp(-2.f*PI*1400.f/Rate);
    const float Rumble=1.f-FMath::Exp(-2.f*PI*90.f/Rate);
    for(int32 I=0;I<NumSamples;++I){
        Gain+=(Target-Gain)*Smooth;
        NoiseState^=NoiseState<<13;NoiseState^=NoiseState>>17;NoiseState^=NoiseState<<5;
        const float Noise=float(NoiseState&0xffffff)/8388607.5f-1.f;
        Low+=Filter*(Noise-Low);Slow+=Rumble*(Low-Slow);
        Phase+=2.*PI*117./Rate;if(Phase>2.*PI)Phase-=2.*PI;
        OutAudio[I]=Gain*((Low-Slow)*.085f+FMath::Sin(Phase)*.009f+FMath::Sin(Phase*2)*.003f);
    }
    return NumSamples;
}
