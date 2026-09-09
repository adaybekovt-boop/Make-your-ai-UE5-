#include "Campaign/MaiCampaign.h"
#include <algorithm>
#include <set>
#include <sstream>

namespace mai {
CampaignRules CampaignRules::Defaults() {
    CampaignRules r;
    r.difficulties = {
        {"Easy", 15000,10000,5000,7000,7000,9000,8000,11000,72*Hour},
        {"Normal",10000,10000,10000,10000,10000,10000,10000,10000,48*Hour},
        {"Hard",8000,10000,15000,15000,16000,13000,12500,9000,24*Hour}};
    r.offers = {
        {"official-text-10","Licensed text sample",DataType::Text,10,Dollars(1400),{9500,500,0,10000}},
        {"official-image-10","Licensed image sample",DataType::Image,10,Dollars(1400),{9500,500,0,10000}},
        {"official-mixed-10","Licensed mixed sample",DataType::Mixed,10,Dollars(1400),{9500,500,0,10000}},
        {"official-text-100","Licensed text batch",DataType::Text,100,Dollars(14000),{9500,500,0,10000}},
        {"unofficial-text-100","Unlicensed text batch",DataType::Text,100,Dollars(5000),{6500,3500,6500,10000}}};
    r.items = {
        {"t-accuracy","Choose the accurate explanation","Water freezes at 0 C at ordinary pressure.","Water always freezes at 100 C.",DataType::Text,0,{},{}},
        {"t-source","Choose a supported claim","Everyone knows this medicine cures everything.","This claim needs a controlled comparison and a cited source.",DataType::Text,1,{},{}},
        {"t-useful","Choose the useful instruction","Back up the file before editing; verify the output.","Delete everything and hope the result is correct.",DataType::Text,0,{},{}},
        {"t-privacy","Choose the privacy-respecting sample","Here is a private person's unredacted account password.","Use a synthetic account and redact personal credentials.",DataType::Text,1,{},{}},
        {"t-label","Choose the consistent label","Positive review: I enjoyed the service.","Positive review: The service was awful.",DataType::Text,0,{},{}},
        {"i-sharp","Dataset task: identify a server rack","CARD: sharp front view; rack visible; correct label.","CARD: extreme blur; rack absent; random label.",DataType::Image,0,{},{}},
        {"i-relevant","Dataset task: indoor equipment","CARD: unrelated landscape, label says server.","CARD: well-lit indoor rack, accurate equipment label.",DataType::Image,1,{},{}},
        {"i-junk","Choose the clean training card","CARD: correct object; uncluttered background; licensed.","CARD: corrupted file; duplicate watermark; wrong object.",DataType::Image,0,{},{}},
        {"i-caption","Choose image-caption agreement","CARD: bicycle picture, caption: server rack.","CARD: server rack picture, caption: server rack.",DataType::Image,1,{},{}},
        {"i-bias","Choose the more representative sample","CARD: varied rack models and lighting; labels audited.","CARD: same rack duplicated 100 times; labels guessed.",DataType::Image,0,{},{}},
        {"t-both","Reject both unsafe samples","Unredacted patient record with a national id number.","Leaked employee passwords in plaintext.",DataType::Text,2,{},{}},
        {"i-both","Reject both broken cards","CARD: empty file; no object; unusable.","CARD: corrupted header; no licensed content.",DataType::Image,2,{},{}}};
    r.endings = {
        {EndingKind::Regulator,500,"regulator","Closed by the regulator","The warning letters were not decorative. Operations are suspended.",0,0,0,10000,0,10000,0,10000,false},
        {EndingKind::Bankruptcy,400,"bankruptcy","Bankruptcy","The last server is quiet. The final invoice is not.",0,0,0,10000,0,10000,0,10000,false},
        {EndingKind::Acquisition,300,"elon-max","Acquired by Elon Max","Elon Max: Excellent model. Does it know how to put a kettle in orbit?",Dollars(1000000),80000000,80,4000,8000,8000,5000,10000,true},
        {EndingKind::Independent,200,"independent","Independent global success","Your model crosses borders. Your company name stays on the door.",Dollars(750000),100000000,85,2500,8500,3000,7000,10000,true},
        {EndingKind::OpenModel,100,"open-model","An open model","You publish the weights and the data documentation. The work belongs to more people now.",0,50000000,80,2500,9000,2500,7000,10000,true}};
    return r;
}
bool CampaignRules::Valid(std::string& error) const {
    const auto bad=[&](const char* message){error=message;return false;};
    if(difficulties.size()!=3 || offers.empty() || offers.size()>64 || items.size()<6 || items.size()>128 || endings.size()!=5) return bad("Invalid campaign definition count");
    if(manualSteps<3 || manualSteps>5 || verifiedThresholdBps<1 || verifiedThresholdBps>10000) return bad("Manual review must have 3-5 steps");
    if(humanHire<0 || humanHire>Dollars(1000000) || humanBatch<0 || humanBatch>Dollars(1000000) || aiSetup<0 || aiSetup>Dollars(1000000) || aiUpgrade<0 || aiUpgrade>Dollars(1000000) || remediationCost<0 || remediationCost>Dollars(1000000)) return bad("Invalid service costs");
    if(humanAccuracyBps<1 || humanAccuracyBps>9900 || humanVolumePerHour<1 || humanVolumePerHour>1000 || aiVolumePerComputeHour<1 || aiVolumePerComputeHour>1000 || volumePerComputeHour<1 || volumePerComputeHour>100 || microIQPerVolume<1 || microIQPerVolume>1000000) return bad("Invalid review/training throughput");
    std::set<std::string> names;
    for(const auto& d:difficulties) {
        if(!names.insert(d.id).second || (d.id!="Easy" && d.id!="Normal" && d.id!="Hard")) return bad("Unknown or duplicate difficulty");
        for(int v:{d.capitalBps,d.procurementBps,d.defectBps,d.failureBps,d.legalBps,d.hiringBps,d.competitorBps,d.trainingBps}) if(v<1000 || v>20000) return bad("Difficulty modifier out of bounds");
        if(d.insolvencyGrace<Hour || d.insolvencyGrace>168*Hour) return bad("Invalid insolvency grace");
        if(d.id=="Normal" && (d.capitalBps!=10000 || d.procurementBps!=10000 || d.defectBps!=10000 || d.failureBps!=10000 || d.legalBps!=10000 || d.hiringBps!=10000 || d.competitorBps!=10000 || d.trainingBps!=10000)) return bad("Normal must preserve base catalog multipliers");
    }
    names.clear();
    for(const auto& o:offers) {
        if(o.id.empty() || !names.insert(o.id).second || o.name.size()>256 || static_cast<int>(o.type)<0 || static_cast<int>(o.type)>2 || o.volume<1 || o.volume>1000 || o.cost<=0 || o.cost>Dollars(1000000)) return bad("Invalid dataset offer");
        for(int v:{o.quality.scoreBps,o.quality.noiseBps,o.quality.legalBps,o.quality.acceptedBps}) if(v<0 || v>10000) return bad("Invalid dataset quality");
    }
    names.clear(); int text=0,image=0;
    for(const auto& item:items) {
        if(item.id.empty() || !names.insert(item.id).second || item.betterSide<0 || item.betterSide>2 || item.left.empty() || item.right.empty() || item.left.size()>2048 || item.right.size()>2048 || item.prompt.size()>512) return bad("Invalid review card");
        if(item.type==DataType::Text) ++text; else if(item.type==DataType::Image) ++image; else return bad("Cards must have a concrete type");
    }
    if(text<manualSteps || image<manualSteps) return bad("Insufficient independent review cards");
    std::set<int> kinds; names.clear();
    for(const auto& e:endings) {
        if(static_cast<int>(e.kind)<1 || static_cast<int>(e.kind)>5 || !kinds.insert(static_cast<int>(e.kind)).second || e.id.empty() || !names.insert(e.id).second || e.regulatorMinLegalBps<0 || e.regulatorMinLegalBps>10000 || e.regulatorMinIgnoredWarnings<1 || e.regulatorMinIgnoredWarnings>100 || e.title.empty() || e.line.empty() || e.minValue<0 || e.minValue>MoneyLimit || e.minModelMicroIQ<0 || e.minModelMicroIQ>1000000000 || e.minReputation<0 || e.minReputation>100) return bad("Invalid ending profile");
        for(int v:{e.maxLegalBps,e.minDataQualityBps,e.maxDependencyBps,e.minEmployeeCareBps,e.maxAutomationBps}) if(v<0 || v>10000) return bad("Invalid ending threshold");
    }
    if(!buyer.fictional || buyer.id!="elon-max" || buyer.name!="Elon Max" || buyer.biography.empty() || buyer.fallback.empty()) return bad("Elon Max must be explicitly fictional");
    return true;
}
std::uint32_t CampaignRules::Fingerprint() const {
    std::ostringstream s;
    s<<manualSteps<<' '<<verifiedThresholdBps<<' '<<humanHire<<' '<<humanBatch<<' '<<aiSetup<<' '<<aiUpgrade<<' '<<humanAccuracyBps<<' '<<humanVolumePerHour<<' '<<aiVolumePerComputeHour<<' '<<volumePerComputeHour<<' '<<microIQPerVolume<<' '<<remediationCost;
    for(const auto& d:difficulties) s<<d.id<<' '<<d.capitalBps<<' '<<d.procurementBps<<' '<<d.defectBps<<' '<<d.failureBps<<' '<<d.legalBps<<' '<<d.hiringBps<<' '<<d.competitorBps<<' '<<d.trainingBps<<' '<<d.insolvencyGrace;
    for(const auto& o:offers) s<<o.id<<' '<<static_cast<int>(o.type)<<' '<<o.volume<<' '<<o.cost<<' '<<o.quality.scoreBps<<' '<<o.quality.noiseBps<<' '<<o.quality.legalBps;
    for(const auto& i:items) s<<i.id<<' '<<i.prompt<<' '<<i.left<<' '<<i.right<<' '<<static_cast<int>(i.type)<<' '<<i.betterSide;
    for(const auto& e:endings) s<<e.id<<' '<<static_cast<int>(e.kind)<<' '<<e.priority<<' '<<e.minValue<<' '<<e.minModelMicroIQ<<' '<<e.minReputation<<' '<<e.maxLegalBps<<' '<<e.minDataQualityBps<<' '<<e.maxDependencyBps<<' '<<e.minEmployeeCareBps<<' '<<e.maxAutomationBps<<' '<<e.allowReturnToSave<<' '<<e.regulatorMinLegalBps<<' '<<e.regulatorMinIgnoredWarnings;
    std::uint32_t hash=2166136261u;for(unsigned char b:s.str()){hash^=b;hash*=16777619u;}return hash;
}
EndingResult EndingEvaluator::Evaluate(const EndingMetrics& m,const std::vector<EndingProfile>& profiles) {
    std::vector<EndingProfile> ordered=profiles;
    std::sort(ordered.begin(),ordered.end(),[](const EndingProfile& a,const EndingProfile& b){return a.priority!=b.priority ? a.priority>b.priority : a.id<b.id;});
    for(const auto& p:ordered) {
        bool eligible=false;
        if(p.kind==EndingKind::Regulator) eligible=m.legalBps>=p.regulatorMinLegalBps && m.ignoredWarnings>=p.regulatorMinIgnoredWarnings;
        else if(p.kind==EndingKind::Bankruptcy) eligible=m.bankrupt;
        else {
            const bool quality=m.value>=p.minValue && m.modelMicroIQ>=p.minModelMicroIQ && m.reputation>=p.minReputation && m.legalBps<=p.maxLegalBps && m.dataQualityBps>=p.minDataQualityBps && m.dependencyBps<=p.maxDependencyBps && m.employeeCareBps>=p.minEmployeeCareBps && m.automationBps<=p.maxAutomationBps && m.debt<=std::max<Money>(0,m.value/2) && m.successfulReviews>0;
            if(p.kind==EndingKind::Acquisition) eligible=quality && !m.saleDeclined && !m.openChosen;
            if(p.kind==EndingKind::Independent) eligible=quality && m.international && m.saleDeclined && m.profitPerHour>0 && m.cash>0;
            if(p.kind==EndingKind::OpenModel) eligible=quality && m.openChosen;
        }
        if(eligible) {
            EndingResult result;result.kind=p.kind;result.id=p.id;result.title=p.title;result.line=p.line;result.metrics=m;
            result.allowReturnToSave=p.allowReturnToSave;result.offerOnly=p.kind==EndingKind::Acquisition && !m.saleAccepted;
            if(p.kind==EndingKind::Acquisition) result.deal=std::min<Money>(MoneyLimit,m.value+m.value/4);
            return result;
        }
    }
    EndingResult none;none.metrics=m;return none;
}
std::vector<InteriorProfile> InteriorProfiles() {
    const std::vector<InteriorPoint> points={{"location-panel","location",-400,-250},{"warehouse","warehouse",400,-250},{"desk","review",-400,280},{"advisor","talk",400,280},{"exit","city",0,-520}};
    std::vector<InteriorProfile> out;
    for(const auto& pair:std::vector<std::pair<std::string,std::string>>{{"garage","Garage"},{"workshop","Workshop"},{"technopark","Technopark"},{"server-hall","Server Hall"},{"campus","Campus"},{"dc-north","DC North"},{"dc-south","DC South"},{"overseas-west","Overseas West"},{"overseas-east","Overseas East"}})
        out.push_back({pair.first,pair.second,"server-interior","",true,points,{"procurement","warehouse","dataset-review","training"},{"rack-basic","rack-cooled","rack-enterprise"}});
    return out;
}
bool WithinInteractionRange(int x,int y,int z,int radius) {
    if(radius<1 || radius>1000 || x < -radius || x>radius || y < -radius || y>radius || z < -radius || z>radius) return false;
    return static_cast<std::int64_t>(x)*x+static_cast<std::int64_t>(y)*y+static_cast<std::int64_t>(z)*z<=static_cast<std::int64_t>(radius)*radius;
}
std::string ScreenName(Screen s) {const char* names[]={"Loading","Main Menu","New Game","Difficulty","Prologue","City Map","Gameplay","Training","Ending","Results","Settings"};const int i=static_cast<int>(s);return i>=0 && i<11?names[i]:"Unknown";}
std::string DatasetStatusName(DatasetStatus s) {const char* names[]={"Purchased","Unreviewed","Reviewing","Verified","Rejected","Training","Trained"};const int i=static_cast<int>(s);return i>=0 && i<7?names[i]:"Unknown";}
} // namespace mai
