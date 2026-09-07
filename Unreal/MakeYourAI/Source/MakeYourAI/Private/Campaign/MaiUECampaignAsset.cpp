#include "Campaign/MaiCampaignAsset.h"
UMaiCampaignAsset::UMaiCampaignAsset() {
    const auto R = mai::CampaignRules::Defaults();
    for (const auto& D : R.difficulties) {
        FMaiDifficultyDefinition V; V.Id = UTF8_TO_TCHAR(D.id.c_str()); V.CapitalBps=D.capitalBps; V.ProcurementBps=D.procurementBps;
        V.DefectBps=D.defectBps; V.FailureBps=D.failureBps; V.LegalBps=D.legalBps; V.HiringBps=D.hiringBps;
        V.CompetitorBps=D.competitorBps; V.TrainingBps=D.trainingBps; V.InsolvencyGraceHours=static_cast<int32>(D.insolvencyGrace/mai::Hour); Difficulties.Add(V);
    }
    for (const auto& O : R.offers) {
        FMaiDatasetOfferDefinition V; V.Id=UTF8_TO_TCHAR(O.id.c_str()); V.Name=UTF8_TO_TCHAR(O.name.c_str()); V.DataType=static_cast<int32>(O.type);
        V.Volume=O.volume; V.CostMicro=O.cost; V.QualityBps=O.quality.scoreBps; V.NoiseBps=O.quality.noiseBps; V.LegalBps=O.quality.legalBps; DatasetOffers.Add(V);
    }
    for (const auto& I : R.items) {
        FMaiReviewCardDefinition V; V.Id=UTF8_TO_TCHAR(I.id.c_str()); V.Prompt=UTF8_TO_TCHAR(I.prompt.c_str()); V.Left=UTF8_TO_TCHAR(I.left.c_str());
        V.Right=UTF8_TO_TCHAR(I.right.c_str()); V.DataType=static_cast<int32>(I.type); V.BetterSide=I.betterSide; ReviewCards.Add(V);
    }
    for (const auto& E : R.endings) {
        FMaiEndingDefinition V; V.Kind=static_cast<int32>(E.kind); V.Priority=E.priority; V.Id=UTF8_TO_TCHAR(E.id.c_str()); V.Title=UTF8_TO_TCHAR(E.title.c_str()); V.Line=UTF8_TO_TCHAR(E.line.c_str());
        V.MinValueMicro=E.minValue; V.MinModelMicroIQ=E.minModelMicroIQ; V.MinReputation=E.minReputation; V.MaxLegalBps=E.maxLegalBps; V.MinDataQualityBps=E.minDataQualityBps;
        V.MaxDependencyBps=E.maxDependencyBps; V.MinEmployeeCareBps=E.minEmployeeCareBps; V.MaxAutomationBps=E.maxAutomationBps; V.bAllowReturnToSave=E.allowReturnToSave; V.RegulatorMinLegalBps=E.regulatorMinLegalBps; V.RegulatorMinIgnoredWarnings=E.regulatorMinIgnoredWarnings; Endings.Add(V);
    }
}
bool UMaiCampaignAsset::ToDomain(mai::CampaignRules& Out, FString& Error) const {
    Out=mai::CampaignRules::Defaults(); Out.difficulties.clear(); Out.offers.clear(); Out.items.clear(); Out.endings.clear();
    for (const auto& V : Difficulties) Out.difficulties.push_back({TCHAR_TO_UTF8(*V.Id),V.CapitalBps,V.ProcurementBps,V.DefectBps,V.FailureBps,V.LegalBps,V.HiringBps,V.CompetitorBps,V.TrainingBps,static_cast<mai::Tick>(V.InsolvencyGraceHours)*mai::Hour});
    for (const auto& V : DatasetOffers) Out.offers.push_back({TCHAR_TO_UTF8(*V.Id),TCHAR_TO_UTF8(*V.Name),static_cast<mai::DataType>(V.DataType),V.Volume,V.CostMicro,{V.QualityBps,V.NoiseBps,V.LegalBps,10000}});
    for (const auto& V : ReviewCards) Out.items.push_back({TCHAR_TO_UTF8(*V.Id),TCHAR_TO_UTF8(*V.Prompt),TCHAR_TO_UTF8(*V.Left),TCHAR_TO_UTF8(*V.Right),static_cast<mai::DataType>(V.DataType),V.BetterSide,{},{}});
    for (const auto& V : Endings) Out.endings.push_back({static_cast<mai::EndingKind>(V.Kind),V.Priority,TCHAR_TO_UTF8(*V.Id),TCHAR_TO_UTF8(*V.Title),TCHAR_TO_UTF8(*V.Line),V.MinValueMicro,V.MinModelMicroIQ,V.MinReputation,V.MaxLegalBps,V.MinDataQualityBps,V.MaxDependencyBps,V.MinEmployeeCareBps,V.MaxAutomationBps,V.bAllowReturnToSave,V.RegulatorMinLegalBps,V.RegulatorMinIgnoredWarnings});
    Out.manualSteps=ManualSteps; Out.humanHire=HumanHireMicro; Out.humanBatch=HumanBatchMicro; Out.aiSetup=AISetupMicro; Out.aiUpgrade=AIUpgradeMicro;
    std::string Why; const bool Valid=Out.Valid(Why); Error=UTF8_TO_TCHAR(Why.c_str()); return Valid;
}
