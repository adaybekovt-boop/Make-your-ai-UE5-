#include "World/MaiCityBatch.h"
#include "World/MaiPlayerController.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Materials/MaterialInterface.h"
AMaiCityBatch::AMaiCityBatch() {
    Instances=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("LinkedGeometry"));
    RootComponent=Instances;
    Instances->SetMobility(EComponentMobility::Static);
    Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrimaryActorTick.bCanEverTick=false;
}
bool AMaiCityBatch::Configure(UStaticMesh* Mesh,const TArray<UMaterialInterface*>& Materials,const FString& Json,const FString& GameLocation) {
    if(!Mesh || Materials.Num()!=Mesh->GetStaticMaterials().Num() || Instances->GetInstanceCount()!=0)return false;
    for(auto* M:Materials)if(!M)return false;
    TArray<TSharedPtr<FJsonValue>> Values;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Values)||Values.Num()>50000)return false;
    TArray<FTransform> Transforms;Transforms.Reserve(Values.Num());
    for(const auto& Value:Values){const TArray<TSharedPtr<FJsonValue>>* A=nullptr;
        if(!Value->TryGetArray(A)||A->Num()!=16)return false;FMatrix Matrix=FMatrix::Identity;
        // Blender interchange uses column-vector affine matrices; UE uses row vectors.
        for(int R=0;R<4;++R)for(int C=0;C<4;++C){double N=0;if(!(*A)[R*4+C]->TryGetNumber(N)||!FMath::IsFinite(N))return false;Matrix.M[C][R]=N;}
        if(FMath::Abs(Matrix.M[0][3])>1e-8||FMath::Abs(Matrix.M[1][3])>1e-8||FMath::Abs(Matrix.M[2][3])>1e-8||FMath::Abs(Matrix.M[3][3]-1)>1e-8)return false;
        FTransform Transform(Matrix);if(Transform.ContainsNaN()||Matrix.Determinant()<=0)return false;
        const FMatrix RoundTrip=Transform.ToMatrixWithScale();
        for(int R=0;R<4;++R)for(int C=0;C<4;++C)if(FMath::Abs(RoundTrip.M[R][C]-Matrix.M[R][C])>1e-4*(1+FMath::Abs(Matrix.M[R][C])))return false;
        Transforms.Add(Transform);
    }
    Instances->SetStaticMesh(Mesh);for(int I=0;I<Materials.Num();++I)Instances->SetMaterial(I,Materials[I]);
    LocationId=GameLocation;
    // Keep forest silhouettes on the overview; LODs do the work before culling.
    if(ActorHasTag(TEXT("MAI_vegetation")))Instances->SetCullDistances(60000,80000);
    if(!LocationId.IsEmpty()){Instances->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Instances->SetCollisionResponseToAllChannels(ECR_Ignore);Instances->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);}
    Instances->AddInstances(Transforms,false,true,false);
    return Instances->GetInstanceCount()==Transforms.Num();
}
void AMaiCityBatch::Interact_Implementation(APlayerController* Player){if(!LocationId.IsEmpty())if(auto* PC=Cast<AMaiPlayerController>(Player))PC->SelectLocation(LocationId);}
