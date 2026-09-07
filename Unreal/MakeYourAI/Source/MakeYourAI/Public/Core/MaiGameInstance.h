#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Gameplay/MaiCatalogAsset.h"
#include "MaiGameInstance.generated.h"

UCLASS(Config=Game)
class MAKEYOURAI_API UMaiGameInstance : public UGameInstance {
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditDefaultsOnly, Category="MakeYourAI") int32 SessionSeed = 1296124209;
    UPROPERTY(Config, EditDefaultsOnly, Category="MakeYourAI") FSoftObjectPath CatalogPath;
    UMaiCatalogAsset* ResolveCatalog();
private:
    UPROPERTY(Transient) TObjectPtr<UMaiCatalogAsset> Catalog;
};
