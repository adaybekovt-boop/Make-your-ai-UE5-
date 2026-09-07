#include "Core/MaiGameInstance.h"

UMaiCatalogAsset* UMaiGameInstance::ResolveCatalog() {
    if (Catalog) return Catalog;
    if (!CatalogPath.IsNull()) {
        Catalog = Cast<UMaiCatalogAsset>(CatalogPath.TryLoad());
        if (!Catalog) UE_LOG(LogTemp, Error, TEXT("Configured MakeYourAI catalog could not be loaded: %s"), *CatalogPath.ToString());
        return Catalog; // Explicitly requested missing assets must not silently fall back.
    }
    Catalog = NewObject<UMaiCatalogAsset>(this); // Native defaults make graybox boot independent of imported content.
    return Catalog;
}
