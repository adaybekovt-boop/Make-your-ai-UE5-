#include "MaiCityImportLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
#include "StaticMeshCompiler.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "HAL/FileManager.h"
#include "RHI.h"
#include "PhysicsEngine/BodySetup.h"
namespace {
FString Json(const TSharedRef<FJsonObject>& Value) { FString Out; FJsonSerializer::Serialize(Value,TJsonWriterFactory<>::Create(&Out)); return Out; }
FString Fail(const FString& Why) { auto O=MakeShared<FJsonObject>();O->SetBoolField(TEXT("ok"),false);O->SetStringField(TEXT("error"),Why);return Json(O); }
struct Reader {
    const TArray<uint8>& Bytes; int64 Offset=0; bool Valid=true;
    template<class T>T Get() { T Value{};if(Offset+sizeof(T)>Bytes.Num()){Valid=false;return Value;} FMemory::Memcpy(&Value,Bytes.GetData()+Offset,sizeof(T));Offset+=sizeof(T);return Value; }
};
int32 CollapsedTriangles(const FMeshDescription& D) {
    const auto P=FStaticMeshConstAttributes(D).GetVertexPositions();int32 Count=0;
    for(const FTriangleID T:D.Triangles().GetElementIDs()) {
        const auto V=D.GetTriangleVertices(T);
        const FVector3f A=P[V[0]],B=P[V[1]],C=P[V[2]];
        if(A==B||A==C||B==C)++Count;
    }
    return Count;
}
}
FString UMaiCityImportLibrary::AuditCityMesh(UStaticMesh* Mesh) {
    if(!Mesh)return Fail(TEXT("No static mesh"));
    TArray<UStaticMesh*> Pending{Mesh};FStaticMeshCompilingManager::Get().FinishCompilation(Pending);
    auto O=MakeShared<FJsonObject>();O->SetBoolField(TEXT("ok"),true);O->SetStringField(TEXT("asset"),Mesh->GetPathName());
    O->SetNumberField(TEXT("sourceModels"),Mesh->GetNumSourceModels());O->SetNumberField(TEXT("materialSlots"),Mesh->GetStaticMaterials().Num());
    TArray<TSharedPtr<FJsonValue>> LODTriangles;for(int32 Level=0;Level<Mesh->GetNumLODs();++Level)LODTriangles.Add(MakeShared<FJsonValueNumber>(Mesh->GetNumTriangles(Level)));O->SetArrayField(TEXT("renderLODTriangles"),LODTriangles);
    TArray<TSharedPtr<FJsonValue>> SourceTriangles;for(int32 Level=0;Level<Mesh->GetNumSourceModels();++Level){const auto* SourceDescription=Mesh->GetMeshDescription(Level);SourceTriangles.Add(MakeShared<FJsonValueNumber>(SourceDescription?SourceDescription->Triangles().Num():-1));}O->SetArrayField(TEXT("sourceLODTriangles"),SourceTriangles);
    const FMeshDescription* D=Mesh->GetMeshDescription(0);
    O->SetNumberField(TEXT("collapsedSourceTriangles"),D?CollapsedTriangles(*D):0);
    O->SetNumberField(TEXT("sourceLOD0Triangles"),D?D->Triangles().Num():-1);
    O->SetNumberField(TEXT("renderLOD0Triangles"),Mesh->GetNumTriangles(0));O->SetNumberField(TEXT("renderLOD0Vertices"),Mesh->GetNumVertices(0));
    O->SetNumberField(TEXT("naniteTriangles"),Mesh->GetNumNaniteTriangles());O->SetNumberField(TEXT("naniteVertices"),Mesh->GetNumNaniteVertices());
    const auto& N=Mesh->NaniteSettings;
    O->SetNumberField(TEXT("fallbackTargetEnum"),int32(N.FallbackTarget));O->SetBoolField(TEXT("naniteEnabled"),N.bEnabled);O->SetNumberField(TEXT("keepPercentTriangles"),N.KeepPercentTriangles);
    O->SetNumberField(TEXT("trimRelativeError"),N.TrimRelativeError);O->SetNumberField(TEXT("fallbackPercentTriangles"),N.FallbackPercentTriangles);O->SetNumberField(TEXT("fallbackRelativeError"),N.FallbackRelativeError);
    if(Mesh->GetNumSourceModels()) {const auto& S=Mesh->GetSourceModel(0);O->SetNumberField(TEXT("reductionPercentTriangles"),S.ReductionSettings.PercentTriangles);O->SetNumberField(TEXT("reductionMaxDeviation"),S.ReductionSettings.MaxDeviation);O->SetBoolField(TEXT("removeDegenerates"),S.BuildSettings.bRemoveDegenerates);}
    O->SetStringField(TEXT("rhi"),GDynamicRHI?FString(GDynamicRHI->GetName()):TEXT("Unavailable"));O->SetNumberField(TEXT("shaderPlatformEnum"),int32(GMaxRHIShaderPlatform));
    O->SetStringField(TEXT("sourceSHA1"),Mesh->GetOutermost()->GetMetaData().GetValue(Mesh,TEXT("MAI_SourceSHA1")));
    return Json(O);
}
FString UMaiCityImportLibrary::ImportCityMesh(const FString& SourceFile,const FString& PackagePath,const FString& ExpectedSHA1,bool EnableNanite) {
    if(!PackagePath.StartsWith(TEXT("/Game/Generated/CityV4/"))||!FPackageName::IsValidLongPackageName(PackagePath))return Fail(TEXT("Unsafe derivative package path"));
    TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*SourceFile))return Fail(TEXT("Cannot read mesh interchange file"));
    uint8 Digest[20];FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Digest);const FString ActualSHA1=BytesToHex(Digest,20).ToLower();
    if(ActualSHA1!=ExpectedSHA1.ToLower())return Fail(TEXT("Source mesh hash mismatch"));
    if(Bytes.Num()<20||FMemory::Memcmp(Bytes.GetData(),"MAIMSH02",8)!=0)return Fail(TEXT("Invalid mesh header"));
    Reader R{Bytes,8,true};const uint32 Vertices=R.Get<uint32>(),Triangles=R.Get<uint32>(),Slots=R.Get<uint32>();
    const uint64 Required=20ULL+Vertices*12ULL+Triangles*124ULL;
    if(!Vertices||Vertices>10000000||!Triangles||Triangles>10000000||!Slots||Slots>4096||Required!=uint64(Bytes.Num()))return Fail(TEXT("Invalid mesh topology counts or size"));
    const FString ObjectName=FPackageName::GetLongPackageAssetName(PackagePath),ObjectPath=PackagePath+TEXT(".")+ObjectName;
    if(UStaticMesh* Existing=LoadObject<UStaticMesh>(nullptr,*ObjectPath)) {
        if(Existing->GetOutermost()->GetMetaData().GetValue(Existing,TEXT("MAI_SourceSHA1"))!=ActualSHA1||Existing->NaniteSettings.bEnabled!=EnableNanite)return Fail(TEXT("Existing asset differs; use another generated content revision"));
        const auto* Description=Existing->GetMeshDescription(0);
        if(!Description||Description->Triangles().Num()!=int32(Triangles)||Existing->GetNumTriangles(0)!=int32(Triangles)-CollapsedTriangles(*Description))return Fail(TEXT("Existing generated asset topology changed; author edits preserved"));
        return AuditCityMesh(Existing);
    }
    if(FPackageName::DoesPackageExist(PackagePath))return Fail(TEXT("Existing non-mesh package will not be replaced"));
    FMeshDescription Description;FStaticMeshAttributes Attributes(Description);Attributes.Register();
    auto Positions=Attributes.GetVertexPositions();auto Normals=Attributes.GetVertexInstanceNormals();auto Tangents=Attributes.GetVertexInstanceTangents();
    auto Signs=Attributes.GetVertexInstanceBinormalSigns();auto UVs=Attributes.GetVertexInstanceUVs();UVs.SetNumChannels(1);
    auto Colors=Attributes.GetVertexInstanceColors();auto MaterialNames=Attributes.GetPolygonGroupMaterialSlotNames();
    TArray<FVertexID> IDs;IDs.Reserve(Vertices);
    for(uint32 I=0;I<Vertices;++I) {const float X=R.Get<float>(),Y=R.Get<float>(),Z=R.Get<float>();if(!FMath::IsFinite(X)||!FMath::IsFinite(Y)||!FMath::IsFinite(Z))return Fail(TEXT("Non-finite vertex"));const FVertexID V=Description.CreateVertex();IDs.Add(V);Positions[V]=FVector3f(X,Y,Z);}
    TArray<FPolygonGroupID> Groups;for(uint32 I=0;I<Slots;++I){const auto G=Description.CreatePolygonGroup();Groups.Add(G);MaterialNames[G]=FName(*FString::Printf(TEXT("Slot_%u"),I));}
    for(uint32 T=0;T<Triangles;++T) {
        const uint32 Slot=R.Get<uint32>();if(Slot>=Slots)return Fail(TEXT("Material slot outside bindings"));TArray<FVertexInstanceID> Corners;Corners.Reserve(3);
        for(int K=0;K<3;++K) {const uint32 Index=R.Get<uint32>();const float NX=R.Get<float>(),NY=R.Get<float>(),NZ=R.Get<float>(),U=R.Get<float>(),V=R.Get<float>();
            const float CR=R.Get<float>(),CG=R.Get<float>(),CB=R.Get<float>(),CA=R.Get<float>();
            if(!FMath::IsFinite(CR)||!FMath::IsFinite(CG)||!FMath::IsFinite(CB)||!FMath::IsFinite(CA))return Fail(TEXT("Non-finite vertex color"));
            const FVector3f N(NX,NY,NZ);if(Index>=Vertices||!R.Valid||N.ContainsNaN()||!FMath::IsFinite(U)||!FMath::IsFinite(V)||N.SizeSquared()<.5f||N.SizeSquared()>1.5f)return Fail(TEXT("Invalid indexed corner"));
            const auto Corner=Description.CreateVertexInstance(IDs[Index]);Corners.Add(Corner);Normals[Corner]=N;UVs.Set(Corner,0,FVector2f(U,V));Colors[Corner]=FVector4f(CR,CG,CB,CA);
            const FVector3f Axis=FMath::Abs(N.Z)<.99f?FVector3f(0,0,1):FVector3f(0,1,0);Tangents[Corner]=FVector3f::CrossProduct(Axis,N).GetSafeNormal();Signs[Corner]=1;
        }
        Description.CreatePolygon(Groups[Slot],Corners);
    }
    if(!R.Valid||R.Offset!=Bytes.Num()||Description.Triangles().Num()!=int32(Triangles))return Fail(TEXT("Topology was not preserved during MeshDescription creation"));
    // MAIMSH02 stores right-handed geometric winding. MeshDescription uses UE's
    // facing convention. Ask UE to compute face normals and reconcile the whole
    // mesh with the authored split normals; never negate the shading normals.
    FStaticMeshOperations::ComputeTriangleTangentsAndNormals(Description);
    auto FaceNormals=Attributes.GetTriangleNormals();
    int32 Aligned=0,Opposed=0;
    for(const FTriangleID T:Description.Triangles().GetElementIDs()) {
        FVector3f Authored=FVector3f::ZeroVector;
        for(const FVertexInstanceID C:Description.GetTriangleVertexInstances(T))Authored+=Normals[C];
        const float Dot=FVector3f::DotProduct(FaceNormals[T],Authored.GetSafeNormal());
        if(Dot>.1f)++Aligned;else if(Dot<-.1f)++Opposed;
    }
    const bool ReverseFacing=Opposed>Aligned;
    if(ReverseFacing)Description.ReverseAllPolygonFacing();
    UE_LOG(LogTemp,Display,TEXT("MAI_FACE_AUDIT %s aligned=%d opposed=%d reversed=%d"),*ObjectName,Aligned,Opposed,ReverseFacing);
    UPackage* Package=CreatePackage(*PackagePath);UStaticMesh* Mesh=NewObject<UStaticMesh>(Package,*ObjectName,RF_Public|RF_Standalone);
    Mesh->InitResources();Mesh->SetLightingGuid();auto& Source=Mesh->AddSourceModel();
    Source.BuildSettings.bRecomputeNormals=false;Source.BuildSettings.bRecomputeTangents=false;Source.BuildSettings.bRemoveDegenerates=false;
    Source.BuildSettings.bUseFullPrecisionUVs=true;Source.BuildSettings.bUseHighPrecisionTangentBasis=true;Source.BuildSettings.bGenerateLightmapUVs=false;
    Source.ReductionSettings.PercentTriangles=1.0f;Source.ReductionSettings.PercentVertices=1.0f;Source.ReductionSettings.MaxDeviation=0.0f;
    const int32 Collapsed=CollapsedTriangles(Description);
    Mesh->CreateMeshDescription(0,MoveTemp(Description));Mesh->CommitMeshDescription(0);
    // Preserve full source LOD0 nearby; simplify only screen-small distant LODs.
    Mesh->SetAutoComputeLODScreenSize(false);
    Mesh->GetSourceModel(0).ScreenSize.Default=1.f;
    if(Triangles>100){
        for(int32 Level=1;Level<=2;++Level){auto& Lod=Mesh->AddSourceModel();
            Lod.ReductionSettings.BaseLODModel=0;
            Lod.ReductionSettings.PercentTriangles=Level==1?.5f:.2f;
            Lod.ScreenSize.Default=Level==1?.12f:.035f;
            Lod.BuildSettings.bRecomputeNormals=false;
        }
    }
    for(uint32 I=0;I<Slots;++I)Mesh->GetStaticMaterials().Add(FStaticMaterial(nullptr,FName(*FString::Printf(TEXT("Slot_%u"),I))));
    // Baseline is full-detail non-Nanite. Opt-in Nanite also retains an unreduced fallback.
    Mesh->NaniteSettings.bEnabled=EnableNanite;Mesh->NaniteSettings.KeepPercentTriangles=1;Mesh->NaniteSettings.TrimRelativeError=0;
    Mesh->NaniteSettings.FallbackTarget=ENaniteFallbackTarget::PercentTriangles;Mesh->NaniteSettings.FallbackPercentTriangles=1;Mesh->NaniteSettings.FallbackRelativeError=0;
    Mesh->CreateBodySetup();Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;
    Mesh->Build(false);TArray<UStaticMesh*> Pending{Mesh};FStaticMeshCompilingManager::Get().FinishCompilation(Pending);
    if(Mesh->GetNumTriangles(0)!=int32(Triangles)-Collapsed)return Fail(FString::Printf(TEXT("LOD0 mismatch: source=%u collapsed=%d imported=%d; geometry cause still requires investigation"),Triangles,Collapsed,Mesh->GetNumTriangles(0)));
    Package->GetMetaData().SetValue(Mesh,TEXT("MAI_SourceSHA1"),*ActualSHA1);Package->GetMetaData().SetValue(Mesh,TEXT("MAI_GeneratedOwner"),TEXT("CityV4-v2"));
    Package->GetMetaData().SetValue(Mesh,TEXT("MAI_FacingReversed"),ReverseFacing?TEXT("true"):TEXT("false"));
    FAssetRegistryModule::AssetCreated(Mesh);Mesh->MarkPackageDirty();const FString Filename=FPackageName::LongPackageNameToFilename(PackagePath,FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    if(!UPackage::SavePackage(Package,Mesh,*Filename,Args))return Fail(TEXT("Static mesh package save failed"));
    return AuditCityMesh(Mesh);
}

FString UMaiCityImportLibrary::CreateTreeDetail(UStaticMesh* NearMesh,UStaticMesh* OriginalMesh,const FString& PackagePath){
    if(!NearMesh||!OriginalMesh||!PackagePath.StartsWith(TEXT("/Game/Generated/CityV4/"))||!FPackageName::IsValidLongPackageName(PackagePath))return Fail(TEXT("Invalid tree LOD inputs"));
    const auto* Near=NearMesh->GetMeshDescription(0);const auto* Original=OriginalMesh->GetMeshDescription(0);
    if(!Near||!Original||NearMesh->GetStaticMaterials().Num()!=OriginalMesh->GetStaticMaterials().Num())return Fail(TEXT("Tree LOD source/binding mismatch"));
    const FString NearHash=NearMesh->GetOutermost()->GetMetaData().GetValue(NearMesh,TEXT("MAI_SourceSHA1"));
    const FString OriginalHash=OriginalMesh->GetOutermost()->GetMetaData().GetValue(OriginalMesh,TEXT("MAI_SourceSHA1"));
    if(NearHash.IsEmpty()||OriginalHash.IsEmpty())return Fail(TEXT("Only audited generated tree sources are accepted"));
    const FString Signature=NearHash+TEXT(":")+OriginalHash+TEXT(":TreeLOD-v2");
    const FString Name=FPackageName::GetLongPackageAssetName(PackagePath);
    if(auto* Existing=LoadObject<UStaticMesh>(nullptr,*(PackagePath+TEXT(".")+Name))){
        if(Existing->GetOutermost()->GetMetaData().GetValue(Existing,TEXT("MAI_TreeLODs"))!=Signature)return Fail(TEXT("Existing tree derivative differs; retained"));
        return AuditCityMesh(Existing);
    }
    if(FPackageName::DoesPackageExist(PackagePath))return Fail(TEXT("Existing package retained"));
    UPackage* Package=CreatePackage(*PackagePath);auto* Mesh=NewObject<UStaticMesh>(Package,*Name,RF_Public|RF_Standalone);
    Mesh->InitResources();Mesh->SetLightingGuid();Mesh->SetAutoComputeLODScreenSize(false);
    Mesh->GetStaticMaterials()=OriginalMesh->GetStaticMaterials();
    for(int32 Level=0;Level<4;++Level){auto& Model=Mesh->AddSourceModel();
        Model.BuildSettings=NearMesh->GetSourceModel(0).BuildSettings;
        Model.ScreenSize.Default=Level==0?1.f:Level==1?.10f:Level==2?.008f:.003f;
        Model.ReductionSettings.BaseLODModel=Level>=2?2:0;
        Model.ReductionSettings.PercentTriangles=Level==1?.12f:Level==3?.2f:1.f;
        Model.ReductionSettings.PercentVertices=1.f;Model.ReductionSettings.MaxDeviation=0.f;
    }
    Mesh->CreateMeshDescription(0,FMeshDescription(*Near));Mesh->CommitMeshDescription(0);
    Mesh->CreateMeshDescription(2,FMeshDescription(*Original));Mesh->CommitMeshDescription(2);
    Mesh->NaniteSettings.bEnabled=false;
    Mesh->CreateBodySetup();Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;
    Mesh->Build(false);TArray<UStaticMesh*> Pending{Mesh};FStaticMeshCompilingManager::Get().FinishCompilation(Pending);
    if(Mesh->GetNumLODs()!=4||Mesh->GetNumTriangles(0)!=NearMesh->GetNumTriangles(0)||Mesh->GetNumTriangles(2)!=OriginalMesh->GetNumTriangles(0))return Fail(TEXT("Tree LOD topology verification failed"));
    Package->GetMetaData().SetValue(Mesh,TEXT("MAI_TreeLODs"),*Signature);
    Package->GetMetaData().SetValue(Mesh,TEXT("MAI_GeneratedOwner"),TEXT("CityV4-v2"));
    FAssetRegistryModule::AssetCreated(Mesh);Mesh->MarkPackageDirty();
    const FString Filename=FPackageName::LongPackageNameToFilename(PackagePath,FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    if(!UPackage::SavePackage(Package,Mesh,*Filename,Args))return Fail(TEXT("Tree LOD save failed"));
    return AuditCityMesh(Mesh);
}
