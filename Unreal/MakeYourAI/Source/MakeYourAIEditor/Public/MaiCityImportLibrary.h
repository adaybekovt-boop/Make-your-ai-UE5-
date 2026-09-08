#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MaiCityImportLibrary.generated.h"
class UStaticMesh;
UCLASS()
class MAKEYOURAIEDITOR_API UMaiCityImportLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    // All derivatives are confined to /Game/Generated/CityV4. Different existing assets are never overwritten.
    UFUNCTION(BlueprintCallable, Category="MakeYourAI|Import")
    static FString ImportCityMesh(const FString& SourceFile, const FString& PackagePath, const FString& ExpectedSHA1, bool EnableNanite);
    UFUNCTION(BlueprintCallable, Category="MakeYourAI|Import")
    static FString AuditCityMesh(UStaticMesh* Mesh);
};
