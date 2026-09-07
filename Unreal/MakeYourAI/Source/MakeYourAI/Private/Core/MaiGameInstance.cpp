#include "Core/MaiGameInstance.h"
#include "Misc/PackageName.h"

UMaiCatalogAsset* UMaiGameInstance::ResolveCatalog() {
    if (Catalog) return Catalog;
    FSoftObjectPath Requested = CatalogPath;
    if (Requested.IsNull() && FPackageName::DoesPackageExist(TEXT("/Game/Scaffold/Data/DA_ScaffoldCatalog"))) {
        Requested = FSoftObjectPath(TEXT("/Game/Scaffold/Data/DA_ScaffoldCatalog.DA_ScaffoldCatalog"));
    }
    if (!Requested.IsNull()) {
        Catalog = Cast<UMaiCatalogAsset>(Requested.TryLoad());
        if (!Catalog) UE_LOG(LogTemp, Error, TEXT("Configured MakeYourAI catalog could not be loaded: %s"), *Requested.ToString());
        return Catalog;
    }
    Catalog = NewObject<UMaiCatalogAsset>(this);
    return Catalog;
}

UMaiCampaignAsset* UMaiGameInstance::ResolveCampaign() {
    if (Campaign) return Campaign;
    FSoftObjectPath Requested=CampaignPath;
    if (Requested.IsNull() && FPackageName::DoesPackageExist(TEXT("/Game/Scaffold/Data/DA_Campaign"))) Requested=FSoftObjectPath(TEXT("/Game/Scaffold/Data/DA_Campaign.DA_Campaign"));
    if (!Requested.IsNull()) {
        Campaign=Cast<UMaiCampaignAsset>(Requested.TryLoad());
        if (!Campaign) UE_LOG(LogTemp, Error, TEXT("Configured campaign data is missing: %s"), *Requested.ToString());
    } else Campaign=NewObject<UMaiCampaignAsset>(this);
    return Campaign;
}
