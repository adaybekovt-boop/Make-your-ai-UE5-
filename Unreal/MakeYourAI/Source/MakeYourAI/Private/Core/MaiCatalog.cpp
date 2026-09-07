#include "Core/MaiDomain.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <set>
#include <limits>

namespace mai {
Catalog Catalog::Defaults() {
    Catalog c;
    c.chips = {{{"consumer-gpu", "Terra T1", Dollars(2000), Dollars(90), 2000, 1000, {6,2}},
                {"pro-gpu", "Titan X9", Dollars(6000), Dollars(180), 3000, 2400, {8,2}},
                {"accelerator", "Helios HC", Dollars(15000), Dollars(420), 6000, 6000, {10,3}},
                {"flagship", "Zenith Z1", Dollars(40000), Dollars(900), 9000, 14000, {12,4}}}};
    c.chassis = {{{"rack-basic", "Basic rack", Dollars(1000), 1, 10000, 10000, {6,2}},
                  {"rack-cooled", "Cooled rack", Dollars(5000), 2, 8500, 10000, {8,3}},
                  {"rack-enterprise", "Enterprise rack", Dollars(20000), 3, 7500, 11000, {12,4}}}};
    c.regions = {{"home", "Home", 0, 10000, 10000, true, false},
                 {"overseas", "Overseas", Dollars(250000), 12500, 15000, true, false},
                 {"greenhaven", "Greenhaven Suburb", 0, 10000, 10000, false, true}};
    c.locations = {
        {"garage", "Garage", "home", Dollars(1500), Dollars(80), 3,3,3000,true},
        {"workshop", "Workshop", "home", Dollars(6500), Dollars(160), 4,4,8000,true},
        {"technopark", "Technopark", "home", Dollars(18000), Dollars(320), 5,5,18000,true},
        {"server-hall", "Server Hall", "home", Dollars(48000), Dollars(700), 6,6,40000,true},
        {"campus", "Campus", "home", Dollars(120000), Dollars(1500), 8,8,90000,true},
        {"dc-north", "DC North", "home", Dollars(220000), Dollars(2200), 8,8,150000,true},
        {"dc-south", "DC South", "home", Dollars(320000), Dollars(3200), 8,8,220000,true},
        {"overseas-west", "Overseas West", "overseas", Dollars(90000), Dollars(900), 6,6,60000,true},
        {"overseas-east", "Overseas East", "overseas", Dollars(210000), Dollars(1900), 8,8,120000,true},
        {"greenhaven-site", "Greenhaven site [BALANCE_TUNABLE]", "greenhaven", 0,0,0,0,0,false}};
    return c;
}
int Catalog::LocationIndex(const std::string& id) const {
    for (std::size_t i=0; i<locations.size(); ++i) if (locations[i].id==id) return static_cast<int>(i);
    return -1;
}
int Catalog::RegionIndex(const std::string& id) const {
    for (std::size_t i=0; i<regions.size(); ++i) if (regions[i].id==id) return static_cast<int>(i);
    return -1;
}
bool Catalog::Valid(std::string& error) const {
    const auto bad=[&error](const char* text){error=text; return false;};
    const auto price=[](Money p){return p>=0 && p<=Dollars(1000000) && p%Unit==0;};
    if (locations.empty() || locations.size()>32 || regions.empty() || regions.size()>16) return bad("Invalid catalog size");
    if (electricityPerKwh<0 || electricityPerKwh>Dollars(1000) || defectPpm<0 || defectPpm>1000000 || failurePpm<0 || failurePpm>1000000) return bad("Invalid rates");
    std::set<std::string> ids;
    std::int64_t maxComputeMilli=0, maxComputeBps=0;
    for (const auto& d : chips) {
        if (d.id.empty() || !ids.insert(d.id).second || !price(d.price) || !price(d.maintenance) || d.watts<=0 || d.watts>100000 || d.computeMilli<=0 || d.computeMilli>1000000) return bad("Invalid chip definition");
        for(int h:d.deliveryHours) if(h<1 || h>48) return bad("Invalid chip delivery");
        maxComputeMilli=std::max(maxComputeMilli,static_cast<std::int64_t>(d.computeMilli));
    }
    for (const auto& d : chassis) {
        if(d.id.empty() || !ids.insert(d.id).second || !price(d.price) || d.maxChip<0 || d.maxChip>3 || d.failureBps<0 || d.failureBps>10000 || d.computeBps<1 || d.computeBps>20000) return bad("Invalid chassis definition");
        for(int h:d.deliveryHours) if(h<1 || h>48) return bad("Invalid rack delivery");
        maxComputeBps=std::max(maxComputeBps,static_cast<std::int64_t>(d.computeBps));
    }
    for(const auto& d:regions) if(d.id.empty() || !ids.insert(d.id).second || !price(d.unlockPrice) || d.tariffBps<1 || d.tariffBps>30000 || d.courtBps<0 || d.courtBps>30000) return bad("Invalid region definition");
    if(RegionIndex("home")<0 || !regions[static_cast<std::size_t>(RegionIndex("home"))].configured) return bad("Home region missing");
    for(const auto& d:locations) {
        if(d.id.empty() || !ids.insert(d.id).second || RegionIndex(d.region)<0 || !price(d.price) || !price(d.rent)) return bad("Invalid location definition");
        if(d.rows<0 || d.cols<0 || d.rows>10 || d.cols>10 || d.rows*d.cols>100 || d.powerWatts<0 || d.powerWatts>1000000) return bad("Invalid location grid");
        if(d.configured && (d.rows==0 || d.cols==0 || d.powerWatts==0)) return bad("Configured location needs grid and power");
        // Bound the cross-product in fixed-point overload throttling. Validate
        // combinations, not merely each tunable independently. Source values are unchanged.
        const std::int64_t maximumComputeMicro=maxComputeMilli*1500*maxComputeBps/10000*d.rows*d.cols;
        const std::int64_t maximumSupplyMilliWatts=static_cast<std::int64_t>(d.powerWatts)*1000;
        if(maximumSupplyMilliWatts>0 && maximumComputeMicro>std::numeric_limits<std::int64_t>::max()/maximumSupplyMilliWatts) return bad("Catalog exceeds fixed-point compute/power arithmetic budget");
    }
    const auto& x=extensions;
    if(!price(x.nuclearPrice) || x.nuclearTariffBps<1 || x.nuclearTariffBps>10000) return bad("Invalid nuclear tariff");
    if(x.nuclearConfigured && x.nuclearPrice==0) return bad("Nuclear price is BALANCE_TUNABLE, not a free purchase");
    if(x.auctionConfigured && (x.auctionReserve<=0 || x.auctionReserve>Dollars(1000000) || x.bidIncrement<=0 || x.bidIncrement>Dollars(1000000) || x.rivalBudget<0 || x.rivalBudget>Dollars(1000000) || x.auctionDuration<=0 || x.auctionDuration>24*Hour || x.rivalInterval<=0 || x.rivalInterval>x.auctionDuration || x.rivalAggressionPpm<0 || x.rivalAggressionPpm>1000000 || x.auctionRack<0 || x.auctionRack>2 || x.auctionChip<0 || x.auctionChip>3 || x.auctionChip>chassis[static_cast<std::size_t>(x.auctionRack)].maxChip || x.auctionQty<1 || x.auctionQty>24 || (x.auctionChannel!=Channel::Official && x.auctionChannel!=Channel::Grey))) return bad("Invalid auction configuration");
    return true;
}
std::uint32_t Catalog::Fingerprint() const {
    std::ostringstream out;
    out<<"MAI-CATALOG-1 "<<electricityPerKwh<<' '<<defectPpm<<' '<<failurePpm;
    for(const auto& d:chips) out<<' '<<d.id<<' '<<d.price<<' '<<d.maintenance<<' '<<d.watts<<' '<<d.computeMilli<<' '<<d.deliveryHours[0]<<' '<<d.deliveryHours[1];
    for(const auto& d:chassis) out<<' '<<d.id<<' '<<d.price<<' '<<d.maxChip<<' '<<d.failureBps<<' '<<d.computeBps<<' '<<d.deliveryHours[0]<<' '<<d.deliveryHours[1];
    for(const auto& d:locations) out<<' '<<d.id<<' '<<d.region<<' '<<d.price<<' '<<d.rent<<' '<<d.rows<<' '<<d.cols<<' '<<d.powerWatts<<' '<<d.configured;
    for(const auto& d:regions) out<<' '<<d.id<<' '<<d.unlockPrice<<' '<<d.tariffBps<<' '<<d.courtBps<<' '<<d.configured;
    const auto& x=extensions;
    out<<' '<<x.nuclearConfigured<<' '<<x.nuclearPrice<<' '<<x.nuclearTariffBps<<' '<<x.auctionConfigured<<' '<<x.auctionReserve<<' '<<x.bidIncrement<<' '<<x.rivalBudget<<' '<<x.auctionDuration<<' '<<x.rivalInterval<<' '<<x.rivalAggressionPpm<<' '<<x.auctionRack<<' '<<x.auctionChip<<' '<<x.auctionQty<<' '<<static_cast<int>(x.auctionChannel);
    std::uint32_t hash=2166136261u;
    for(unsigned char byte:out.str()) {hash^=byte; hash*=16777619u;}
    return hash;
}
std::string FormatMoney(Money money) {
    const bool negative=money<0;
    const std::uint64_t absolute=negative ? static_cast<std::uint64_t>(-(money+1))+1 : static_cast<std::uint64_t>(money);
    std::ostringstream out;
    if(negative) out<<'-';
    out<<'$'<<absolute/Unit<<'.'<<std::setfill('0')<<std::setw(2)<<(absolute%Unit)/(Unit/100);
    return out.str();
}
bool UpdateProximity(NpcState& n, bool inside, Tick now, Tick duration, Tick cooldown) {
    if(duration<=0 || duration>TimeLimit/2 || cooldown<0 || cooldown>TimeLimit || now<0 || now>TimeLimit || cooldown<2*duration || now>TimeLimit-cooldown || n.eventCount<0 || n.eventCount>=1000000000) return false;
    if(n.mode==NpcMode::PhoneCall && now>=n.stageEnds) {n.mode=NpcMode::React; n.stageEnds+=duration;}
    if(n.mode==NpcMode::React && now>=n.stageEnds) n.mode=NpcMode::Idle;
    const bool trigger=inside && !n.wasInside && n.mode==NpcMode::Idle && now>=n.cooldownEnds;
    n.wasInside=inside;
    if(trigger) {n.mode=NpcMode::PhoneCall; n.stageEnds=now+duration; n.cooldownEnds=now+cooldown; ++n.eventCount;}
    return trigger;
}
} // namespace mai
