#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Gameplay/MaiCatalogAsset.h"
#include "Campaign/MaiCampaignAsset.h"
#include "MaiGameInstance.generated.h"

UCLASS(Config=Game)
class MAKEYOURAI_API UMaiGameInstance : public UGameInstance {
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditDefaultsOnly, Category="MakeYourAI") int32 SessionSeed = 1296124209;
    UPROPERTY(Config, EditDefaultsOnly, Category="MakeYourAI") FSoftObjectPath CatalogPath;
    UMaiCatalogAsset* ResolveCatalog();
    UPROPERTY(Config, EditDefaultsOnly, Category="MakeYourAI") FSoftObjectPath CampaignPath;
    UMaiCampaignAsset* ResolveCampaign();
private:
    UPROPERTY(Transient) TObjectPtr<UMaiCatalogAsset> Catalog;
    UPROPERTY(Transient) TObjectPtr<UMaiCampaignAsset> Campaign;
};
