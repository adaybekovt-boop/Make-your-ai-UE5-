#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MaiCityLighting.generated.h"
class ADirectionalLight;
class ASkyLight;
class UMaterialInstanceDynamic;
UCLASS()
class MAKEYOURAI_API AMaiCityLighting : public AActor {
    GENERATED_BODY()
public:
    AMaiCityLighting();
    static float ExposureForSunLux(float Lux) { return FMath::Log2(FMath::Max(.4f, Lux) / 2.5f); }
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ADirectionalLight> Sun;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ASkyLight> Sky;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool PreviewNight=false;
    UFUNCTION(BlueprintCallable,Category="MakeYourAI|Lighting") void ApplyPreview(bool Night);
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> WindowMaterials;
    TArray<float> WindowEmissionStrengths;
    void ApplyHour(double Hour);
    double LastHour=-1;
};
