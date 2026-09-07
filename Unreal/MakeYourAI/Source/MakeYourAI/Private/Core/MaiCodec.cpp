#include "Core/MaiDomain.h"
#include <iomanip>
#include <sstream>
#include <utility>

namespace mai {
namespace {
std::uint32_t Checksum(const std::string& data) {std::uint32_t h=2166136261u;for(unsigned char b:data){h^=b;h*=16777619u;}return h;}
bool Count(std::istream& input,std::size_t& count,std::size_t limit) {return static_cast<bool>(input>>count) && count<=limit;}
}
std::string Simulation::Save() const {
    const auto& s=state_;
    std::ostringstream out;
    out<<s.cash<<' '<<s.capex<<' '<<s.expenses<<' '<<s.revenue<<' '<<s.expenseCarry<<' '<<s.revenueCarry<<' '
       <<s.now<<' '<<s.restrictedUntil<<' '<<s.paused<<' '<<s.ended<<' '<<s.nuclearOwned<<' '<<s.dirtyHistory<<' '
       <<s.speed<<' '<<s.reputation<<' '<<s.usersMicro<<' '<<s.orderSequence<<' '<<s.equipmentRng<<' '<<s.auctionRng<<' '<<s.eventRng<<' '<<std::quoted(s.activeRegion)<<'\n';
    out<<s.unlockedRegions.size()<<'\n';for(const auto& id:s.unlockedRegions) out<<std::quoted(id)<<'\n';
    out<<s.locations.size()<<'\n';
    for(const auto& l:s.locations) {
        out<<std::quoted(l.id)<<' '<<l.owned<<' '<<l.serverSequence<<'\n';
        for(const auto& v:l.chassisStock) out<<v.total<<' '<<v.grey<<' ';out<<'\n';
        for(const auto& v:l.chipStock) out<<v.total<<' '<<v.grey<<' ';out<<'\n';
        out<<l.slots.size()<<'\n';for(const auto& v:l.slots) out<<v.chassis<<' '<<v.chip<<' '<<v.overclockMilli<<' '<<v.serverId<<'\n';
    }
    out<<s.orders.size()<<'\n';
    for(const auto& o:s.orders) out<<o.id<<' '<<std::quoted(o.location)<<' '<<static_cast<int>(o.kind)<<' '<<static_cast<int>(o.channel)<<' '<<o.item<<' '<<o.quantity<<' '<<o.targetCell<<' '<<o.paid<<' '<<o.arrives<<'\n';
    const auto& a=s.auction;
    out<<static_cast<int>(a.phase)<<' '<<std::quoted(a.location)<<' '<<std::quoted(a.rival)<<' '<<a.chassis<<' '<<a.chip<<' '<<a.quantity<<' '<<static_cast<int>(a.channel)<<' '
       <<a.currentBid<<' '<<a.escrow<<' '<<a.increment<<' '<<a.rivalBudget<<' '<<a.reserve<<' '<<a.playerLeading<<' '<<a.closes<<' '<<a.rivalAt<<' '<<a.interval<<' '<<a.aggressionPpm<<'\n';
    out<<static_cast<int>(s.npc.mode)<<' '<<s.npc.stageEnds<<' '<<s.npc.cooldownEnds<<' '<<s.npc.wasInside<<' '<<s.npc.eventCount<<'\n';
    out<<s.notices.size()<<'\n';for(const auto& n:s.notices) out<<std::quoted(n)<<'\n';
    const std::string body=out.str();
    std::ostringstream envelope;envelope<<"MAI-SAVE "<<SaveVersion<<' '<<catalog_.Fingerprint()<<' '<<Checksum(body)<<'\n'<<body;
    return envelope.str();
}
Result Simulation::Load(const std::string& encoded) {
    if(encoded.size()>1024*1024) return Result::Error("Save exceeds size limit");
    const auto newline=encoded.find('\n');if(newline==std::string::npos) return Result::Error("Missing save header");
    std::istringstream header(encoded.substr(0,newline));std::string magic;int version=0;std::uint32_t fingerprint=0,checksum=0;
    if(!(header>>magic>>version>>fingerprint>>checksum) || magic!="MAI-SAVE" || version!=SaveVersion) return Result::Error("Unsupported save schema (browser saves are not UE saves)");
    header>>std::ws;if(!header.eof()) return Result::Error("Unexpected header data");
    if(fingerprint!=catalog_.Fingerprint()) return Result::Error("Catalog changed; explicit save migration required");
    const std::string body=encoded.substr(newline+1);if(Checksum(body)!=checksum) return Result::Error("Save integrity check failed");
    std::istringstream in(body);State s;std::size_t size=0;
    if(!(in>>s.cash>>s.capex>>s.expenses>>s.revenue>>s.expenseCarry>>s.revenueCarry>>s.now>>s.restrictedUntil>>s.paused>>s.ended>>s.nuclearOwned>>s.dirtyHistory>>s.speed>>s.reputation>>s.usersMicro>>s.orderSequence>>s.equipmentRng>>s.auctionRng>>s.eventRng>>std::quoted(s.activeRegion))) return Result::Error("Truncated company state");
    if(!Count(in,size,16)) return Result::Error("Invalid region count");s.unlockedRegions.clear();
    for(std::size_t i=0;i<size;++i) {std::string id;if(!(in>>std::quoted(id)) || id.size()>128) return Result::Error("Invalid region id");s.unlockedRegions.push_back(id);}
    if(!Count(in,size,32)) return Result::Error("Invalid location count");s.locations.resize(size);
    for(auto& l:s.locations) {
        if(!(in>>std::quoted(l.id)>>l.owned>>l.serverSequence) || l.id.size()>128) return Result::Error("Invalid location record");
        for(auto& v:l.chassisStock) if(!(in>>v.total>>v.grey)) return Result::Error("Truncated chassis inventory");
        for(auto& v:l.chipStock) if(!(in>>v.total>>v.grey)) return Result::Error("Truncated chip inventory");
        std::size_t slots=0;if(!Count(in,slots,100)) return Result::Error("Invalid grid size");l.slots.resize(slots);
        for(auto& v:l.slots) if(!(in>>v.chassis>>v.chip>>v.overclockMilli>>v.serverId)) return Result::Error("Truncated server grid");
    }
    if(!Count(in,size,198)) return Result::Error("Invalid order count");s.orders.resize(size);
    for(auto& o:s.orders) {
        int kind=0,channel=0;
        if(!(in>>o.id>>std::quoted(o.location)>>kind>>channel>>o.item>>o.quantity>>o.targetCell>>o.paid>>o.arrives) || o.location.size()>128 || kind<0 || kind>1 || channel<0 || channel>1) return Result::Error("Invalid order record");
        o.kind=static_cast<Kind>(kind);o.channel=static_cast<Channel>(channel);
    }
    auto& a=s.auction;int phase=0,channel=0,npc=0;
    if(!(in>>phase>>std::quoted(a.location)>>std::quoted(a.rival)>>a.chassis>>a.chip>>a.quantity>>channel>>a.currentBid>>a.escrow>>a.increment>>a.rivalBudget>>a.reserve>>a.playerLeading>>a.closes>>a.rivalAt>>a.interval>>a.aggressionPpm) || phase<0 || phase>3 || channel<0 || channel>1 || a.location.size()>128 || a.rival.size()>256) return Result::Error("Invalid auction record");
    a.phase=static_cast<AuctionPhase>(phase);a.channel=static_cast<Channel>(channel);
    if(!(in>>npc>>s.npc.stageEnds>>s.npc.cooldownEnds>>s.npc.wasInside>>s.npc.eventCount) || npc<0 || npc>2) return Result::Error("Invalid NPC record");s.npc.mode=static_cast<NpcMode>(npc);
    if(!Count(in,size,32)) return Result::Error("Invalid notice count");s.notices.resize(size);
    for(auto& n:s.notices) if(!(in>>std::quoted(n)) || n.size()>2048) return Result::Error("Invalid notice");
    in>>std::ws;if(!in.eof()) return Result::Error("Unexpected trailing save data");
    return Restore(s); // Validate completely before changing the live company.
}
} // namespace mai
