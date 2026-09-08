#pragma once
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace MaiJson {
inline FString String(const TSharedPtr<FJsonObject>& O,const TCHAR* K,const FString& D={}){FString V;return O&&O->TryGetStringField(K,V)?V:D;}
inline double Number(const TSharedPtr<FJsonObject>& O,const TCHAR* K,double D=0){double V;return O&&O->TryGetNumberField(K,V)?V:D;}
inline bool Boolean(const TSharedPtr<FJsonObject>& O,const TCHAR* K,bool D=false){bool V;return O&&O->TryGetBoolField(K,V)?V:D;}
inline TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonObject>& O,const TCHAR* K){const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(K,V)?*V:nullptr;}
inline TArray<TSharedPtr<FJsonValue>> Array(const TSharedPtr<FJsonObject>& O,const TCHAR* K){const TArray<TSharedPtr<FJsonValue>>* V=nullptr;return O&&O->TryGetArrayField(K,V)?*V:TArray<TSharedPtr<FJsonValue>>{};}
inline FString Encode(const TSharedPtr<FJsonObject>& O){FString S;if(O)FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&S));return S;}
inline TSharedPtr<FJsonObject> Decode(const FString& S){TSharedPtr<FJsonObject> O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),O);return O;}
inline auto S(const FString& V){return MakeShared<FJsonValueString>(V);}
inline auto N(double V){return MakeShared<FJsonValueNumber>(V);}
inline TSharedRef<FJsonObject> Request(const TCHAR* Method){auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("method"),Method);return O;}
inline auto Node(const FString& Kind,const FString& Id,const FString& Label={}){auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("kind"),Kind);O->SetStringField(TEXT("id"),Id);O->SetStringField(TEXT("label"),Label);return O;}
inline auto Text(const FString& Id,const FString& Label){return Node(TEXT("text"),Id,Label);}
inline auto Head(const FString& Id,const FString& Label){return Node(TEXT("heading"),Id,Label);}
inline auto Button(const FString& Id,const FString& Label,const FString& Action,const TArray<TSharedPtr<FJsonValue>>& Args={},bool Enabled=true){auto O=Node(TEXT("button"),Id,Label);O->SetStringField(TEXT("action"),Action);O->SetArrayField(TEXT("args"),Args);O->SetBoolField(TEXT("enabled"),Enabled);return O;}
inline void Add(const TSharedRef<FJsonObject>& Parent,const TSharedRef<FJsonObject>& Child){auto A=Array(Parent,TEXT("children"));A.Add(MakeShared<FJsonValueObject>(Child));Parent->SetArrayField(TEXT("children"),A);}
}
