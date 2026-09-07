#include "Core/MaiDomain.h"
#include <algorithm>
#include <set>
#include <utility>

namespace mai {
namespace {
bool ValidChannel(Channel c) {return c==Channel::Official || c==Channel::Grey;}
bool ValidKind(Kind k) {return k==Kind::Chassis || k==Kind::Chip;}
bool Has(const std::vector<std::string>& values,const std::string& id) {return std::find(values.begin(),values.end(),id)!=values.end();}
std::int64_t Scale(std::int64_t a,std::int64_t b,std::int64_t denominator) {
    // Nonnegative bounded inputs. Split first to avoid overflowing a*b.
    return (a/denominator)*b + ((a%denominator)*b)/denominator;
}
std::int64_t PowerMicroWatts(const ChipDef& d,const Slot& s) {return static_cast<std::int64_t>(d.watts)*s.overclockMilli*s.overclockMilli;}
}
Simulation::Simulation(Catalog catalog,std::uint32_t seed):catalog_(std::move(catalog)) {Reset(seed);}
Result Simulation::Reset(std::uint32_t seed) {
    std::string error;
    if(!catalog_.Valid(error)) return Result::Error(error);
    state_=State{};
    state_.equipmentRng=seed ? seed : 1u;
    state_.auctionRng=(seed^0x9e3779b9u) ? (seed^0x9e3779b9u) : 2u;
    state_.eventRng=(seed^0x85ebca6bu) ? (seed^0x85ebca6bu) : 3u;
    for(const auto& definition:catalog_.locations) {
        Location location; location.id=definition.id;
        location.slots.resize(static_cast<std::size_t>(definition.rows*definition.cols));
        state_.locations.push_back(std::move(location));
    }
    Notice("SOURCE_SCAFFOLD: Garage procurement and default flagship economy; not the complete browser simulation.");
    return Result::Success();
}
void Simulation::Notice(const std::string& text) {
    state_.notices.push_back(text);
    if(state_.notices.size()>32) state_.notices.erase(state_.notices.begin());
}
std::uint32_t Simulation::Roll(std::uint32_t& seed) {
    if(seed==0) seed=1;
    seed^=seed<<13; seed^=seed>>17; seed^=seed<<5;
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(seed)*1000000u)>>32);
}
Result Simulation::BuyBlocked() const {
    if(state_.ended) return Result::Error("Company has ended");
    if(state_.now<state_.restrictedUntil) return Result::Error("Purchases restricted by the board");
    return Result::Success();
}
Status Simulation::LocationStatus(const std::string& id) const {
    const int i=catalog_.LocationIndex(id);
    if(i<0 || static_cast<std::size_t>(i)>=state_.locations.size()) return Status::Locked;
    const auto& d=catalog_.locations[static_cast<std::size_t>(i)];
    if(state_.locations[static_cast<std::size_t>(i)].owned) return Status::Owned;
    if(!d.configured || !Has(state_.unlockedRegions,d.region)) return Status::Locked;
    return Status::Available; // No invented sequential unlock on the seven home sites.
}
Result Simulation::BuyLocation(const std::string& id) {
    auto permitted=BuyBlocked(); if(!permitted.ok) return permitted;
    if(LocationStatus(id)!=Status::Available) return Result::Error("Location is locked or already owned");
    const auto i=static_cast<std::size_t>(catalog_.LocationIndex(id));
    const Money cost=catalog_.locations[i].price;
    if(state_.cash<cost || state_.capex>MoneyLimit-cost) return Result::Error("Insufficient funds or accounting limit");
    state_.cash-=cost; state_.capex+=cost; state_.locations[i].owned=true;
    Notice("Purchased "+catalog_.locations[i].name); return Result::Success();
}
Result Simulation::UnlockRegion(const std::string& id) {
    auto permitted=BuyBlocked(); if(!permitted.ok) return permitted;
    const int i=catalog_.RegionIndex(id);
    if(i<0 || Has(state_.unlockedRegions,id)) return Result::Error("Unknown or already unlocked region");
    const auto& d=catalog_.regions[static_cast<std::size_t>(i)];
    if(!d.configured) return Result::Error("BALANCE_TUNABLE: region configuration required");
    if(state_.cash<d.unlockPrice || state_.capex>MoneyLimit-d.unlockPrice) return Result::Error("Insufficient funds or accounting limit");
    state_.cash-=d.unlockPrice; state_.capex+=d.unlockPrice; state_.unlockedRegions.push_back(id);
    Notice("Region unlocked: "+d.name); return Result::Success();
}
Result Simulation::SwitchRegion(const std::string& id) {
    if(!Has(state_.unlockedRegions,id)) return Result::Error("Unlock the region first");
    state_.activeRegion=id; return Result::Success();
}
Result Simulation::BuyNuclear() {
    auto permitted=BuyBlocked(); if(!permitted.ok) return permitted;
    const auto& x=catalog_.extensions;
    if(!x.nuclearConfigured) return Result::Error("BALANCE_TUNABLE: nuclear price and tariff require configuration");
    if(state_.nuclearOwned) return Result::Error("Only one nuclear station per company");
    if(state_.cash<x.nuclearPrice || state_.capex>MoneyLimit-x.nuclearPrice) return Result::Error("Insufficient funds or accounting limit");
    state_.cash-=x.nuclearPrice; state_.capex+=x.nuclearPrice; state_.nuclearOwned=true;
    Notice("Company electricity tariff changed; the station produces no direct income"); return Result::Success();
}
int Simulation::DiscountPercent(int quantity) {return quantity<1 ? 0 : std::min(20,(quantity-1)*5);}
Money Simulation::OrderPrice(Money base,Channel channel,int quantity) {
    if(!ValidChannel(channel) || quantity<1 || quantity>24 || base<0 || base>Dollars(1000000) || base%Unit!=0) return -1;
    // Browser Math.round(base * channel), THEN Math.round(unit * qty * discount).
    const Money unitDollars=((base/Unit)*(channel==Channel::Official ? 16 : 11)+5)/10;
    return ((unitDollars*quantity*(100-DiscountPercent(quantity))+50)/100)*Unit;
}
int Simulation::ReservedOrders() const {return state_.auction.phase==AuctionPhase::Open ? 2 : 0;}
Result Simulation::ValidateOrder(const std::string& id,Kind kind,int item,Channel channel,int quantity,int cell) const {
    auto permitted=BuyBlocked(); if(!permitted.ok) return permitted;
    if(!ValidKind(kind) || !ValidChannel(channel) || quantity<1 || quantity>24 || item<0 || item>=(kind==Kind::Chassis ? 3 : 4)) return Result::Error("Invalid equipment order");
    const int index=catalog_.LocationIndex(id);
    if(index<0 || LocationStatus(id)!=Status::Owned) return Result::Error("Buy the location first");
    const auto& location=state_.locations[static_cast<std::size_t>(index)];
    if(cell < -1 || cell>=static_cast<int>(location.slots.size())) return Result::Error("Cell outside location");
    if(cell>=0) {
        const auto& slot=location.slots[static_cast<std::size_t>(cell)];
        if(kind==Kind::Chassis && slot.chassis>=0) return Result::Error("Cell is occupied");
        if(kind==Kind::Chip) {
            if(slot.chassis<0) return Result::Error("Install a chassis first");
            if(item>catalog_.chassis[static_cast<std::size_t>(slot.chassis)].maxChip) return Result::Error("Incompatible chip");
            if(slot.chip>=0 && catalog_.chips[static_cast<std::size_t>(item)].computeMilli<=catalog_.chips[static_cast<std::size_t>(slot.chip)].computeMilli) return Result::Error("Upgrade must be faster");
        }
    }
    int stock=kind==Kind::Chassis ? location.chassisStock[static_cast<std::size_t>(item)].total : location.chipStock[static_cast<std::size_t>(item)].total;
    for(const auto& order:state_.orders) if(order.location==id && order.kind==kind && order.item==item) stock+=order.quantity;
    if(stock+quantity>1000000) return Result::Error("Warehouse capacity limit");
    return Result::Success();
}
void Simulation::AppendOrder(const std::string& id,Kind kind,int item,Channel channel,int quantity,int cell,Money paid) {
    const int hours=kind==Kind::Chassis ? catalog_.chassis[static_cast<std::size_t>(item)].deliveryHours[static_cast<std::size_t>(channel)] : catalog_.chips[static_cast<std::size_t>(item)].deliveryHours[static_cast<std::size_t>(channel)];
    state_.orders.push_back({++state_.orderSequence,id,kind,channel,item,quantity,cell,paid,state_.now+hours*Hour});
}
Result Simulation::OrderItem(const std::string& id,Kind kind,int item,Channel channel,int quantity,int cell) {
    auto valid=ValidateOrder(id,kind,item,channel,quantity,cell); if(!valid.ok) return valid;
    if(state_.orders.size()+static_cast<std::size_t>(ReservedOrders())+1>198) return Result::Error("Too many pending orders");
    const Money base=kind==Kind::Chassis ? catalog_.chassis[static_cast<std::size_t>(item)].price : catalog_.chips[static_cast<std::size_t>(item)].price;
    const Money price=OrderPrice(base,channel,quantity);
    if(price<0 || state_.cash<price || state_.capex>MoneyLimit-price) return Result::Error("Insufficient funds or accounting limit");
    state_.cash-=price; state_.capex+=price; AppendOrder(id,kind,item,channel,quantity,cell,price);
    Notice("Order paid once; delivery to the location warehouse"); return Result::Success();
}
Result Simulation::OrderKit(const std::string& id,int rack,int chip,Channel channel,int quantity,int cell) {
    if(rack<0 || rack>2 || chip<0 || chip>3 || chip>catalog_.chassis[static_cast<std::size_t>(rack)].maxChip || cell<0) return Result::Error("Incompatible kit or missing cell");
    auto a=ValidateOrder(id,Kind::Chassis,rack,channel,quantity,cell); if(!a.ok) return a;
    auto b=ValidateOrder(id,Kind::Chip,chip,channel,quantity,-1); if(!b.ok) return b;
    if(state_.orders.size()+static_cast<std::size_t>(ReservedOrders())+2>198) return Result::Error("Too many pending orders");
    const Money p1=OrderPrice(catalog_.chassis[static_cast<std::size_t>(rack)].price,channel,quantity);
    const Money p2=OrderPrice(catalog_.chips[static_cast<std::size_t>(chip)].price,channel,quantity);
    if(state_.cash<p1+p2 || state_.capex>MoneyLimit-p1-p2) return Result::Error("Insufficient funds for the whole kit");
    state_.cash-=p1+p2; state_.capex+=p1+p2;
    AppendOrder(id,Kind::Chassis,rack,channel,quantity,cell,p1);
    AppendOrder(id,Kind::Chip,chip,channel,quantity,-1,p2);
    Notice("Kit ordered atomically; installation is manual"); return Result::Success();
}
void Simulation::Deliver() {
    auto it=state_.orders.begin();
    while(it!=state_.orders.end()) {
        if(it->arrives>state_.now) {++it; continue;}
        const int index=catalog_.LocationIndex(it->location);
        if(index<0) {++it; continue;} // Validate rejects this in loaded data; never discard stock silently.
        auto& location=state_.locations[static_cast<std::size_t>(index)];
        auto& stock=it->kind==Kind::Chassis ? location.chassisStock[static_cast<std::size_t>(it->item)] : location.chipStock[static_cast<std::size_t>(it->item)];
        stock.total+=it->quantity; if(it->channel==Channel::Grey) stock.grey+=it->quantity;
        Notice("Delivered to "+it->location+" warehouse; select a cell to install");
        it=state_.orders.erase(it); // No payment and no defect roll at delivery.
    }
}
bool Simulation::Take(Stock& stock) {
    const bool grey=stock.total==stock.grey; // Verified/official equipment first.
    --stock.total; if(grey) --stock.grey; return grey;
}
void Simulation::ChargeFailure(Money price) {
    const Money cleanup=(((price/Unit)*15+50)/100)*Unit;
    state_.cash-=cleanup; state_.expenses+=cleanup;
    Notice("Equipment failed; disposal charged "+FormatMoney(cleanup));
}
Result Simulation::MountChassis(const std::string& id,int cell,int rack) {
    if(state_.ended || LocationStatus(id)!=Status::Owned || rack<0 || rack>2) return Result::Error("Invalid installation");
    auto& location=state_.locations[static_cast<std::size_t>(catalog_.LocationIndex(id))];
    if(cell<0 || cell>=static_cast<int>(location.slots.size()) || location.slots[static_cast<std::size_t>(cell)].chassis>=0) return Result::Error("Cell is unavailable");
    auto& stock=location.chassisStock[static_cast<std::size_t>(rack)];
    if(stock.total<1) return Result::Error("Chassis is not in the warehouse");
    const bool grey=Take(stock);
    if(grey && Roll(state_.equipmentRng)<static_cast<std::uint32_t>(catalog_.defectPpm)) {
        ChargeFailure(catalog_.chassis[static_cast<std::size_t>(rack)].price);
        return Result::Success("Defective grey chassis disposed; cell remains empty");
    }
    location.slots[static_cast<std::size_t>(cell)].chassis=rack;
    return Result::Success("Chassis installed without a second payment");
}
Result Simulation::MountChip(const std::string& id,int cell,int chip) {
    if(state_.ended || LocationStatus(id)!=Status::Owned || chip<0 || chip>3) return Result::Error("Invalid installation");
    const auto index=static_cast<std::size_t>(catalog_.LocationIndex(id));
    auto& location=state_.locations[index];
    if(cell<0 || cell>=static_cast<int>(location.slots.size())) return Result::Error("Cell outside location");
    auto& slot=location.slots[static_cast<std::size_t>(cell)];
    if(slot.chassis<0 || chip>catalog_.chassis[static_cast<std::size_t>(slot.chassis)].maxChip) return Result::Error("Missing or incompatible chassis");
    if(slot.chip>=0 && catalog_.chips[static_cast<std::size_t>(chip)].computeMilli<=catalog_.chips[static_cast<std::size_t>(slot.chip)].computeMilli) return Result::Error("Upgrade must be faster");
    auto& stock=location.chipStock[static_cast<std::size_t>(chip)];
    if(stock.total<1) return Result::Error("Chip is not in the warehouse");
    std::int64_t demand=static_cast<std::int64_t>(catalog_.chips[static_cast<std::size_t>(chip)].watts)*1000000;
    for(std::size_t i=0;i<location.slots.size();++i) if(static_cast<int>(i)!=cell && location.slots[i].chip>=0) demand+=PowerMicroWatts(catalog_.chips[static_cast<std::size_t>(location.slots[i].chip)],location.slots[i]);
    if(demand>static_cast<std::int64_t>(catalog_.locations[index].powerWatts)*1000000) return Result::Error("Insufficient power; stock and RNG unchanged");
    const bool grey=Take(stock);
    if(slot.chip>=0) ++location.chipStock[static_cast<std::size_t>(slot.chip)].total; // Working old chip is verified stock, even when replacement fails.
    if(slot.chip<0) slot.serverId=++location.serverSequence;
    slot.chip=chip; slot.overclockMilli=1000;
    if(grey && Roll(state_.equipmentRng)<static_cast<std::uint32_t>(catalog_.defectPpm)) return FailChip(id,cell);
    return Result::Success("Chip installed; working previous chip returned to warehouse");
}
Result Simulation::FailChip(const std::string& id,int cell) {
    const int i=catalog_.LocationIndex(id);
    if(i<0 || cell<0 || cell>=static_cast<int>(state_.locations[static_cast<std::size_t>(i)].slots.size())) return Result::Error("Invalid server");
    auto& slot=state_.locations[static_cast<std::size_t>(i)].slots[static_cast<std::size_t>(cell)];
    if(slot.chip<0) return Result::Error("No installed chip");
    ChargeFailure(catalog_.chips[static_cast<std::size_t>(slot.chip)].price);
    slot.chip=-1; slot.serverId=0; slot.overclockMilli=1000;
    return Result::Success("Chip failed; chassis retained");
}
Result Simulation::SetOverclock(const std::string& id,int cell,int milli) {
    if(state_.ended || LocationStatus(id)!=Status::Owned || milli<500 || milli>1500) return Result::Error("Overclock must be between 0.5 and 1.5");
    auto& location=state_.locations[static_cast<std::size_t>(catalog_.LocationIndex(id))];
    if(cell<0 || cell>=static_cast<int>(location.slots.size()) || location.slots[static_cast<std::size_t>(cell)].chip<0) return Result::Error("No installed server");
    location.slots[static_cast<std::size_t>(cell)].overclockMilli=milli; return Result::Success();
}
Rates Simulation::LocationEconomy(int index) const {
    Rates result;
    if(index<0 || static_cast<std::size_t>(index)>=state_.locations.size()) return result;
    const auto& location=state_.locations[static_cast<std::size_t>(index)];
    const auto& definition=catalog_.locations[static_cast<std::size_t>(index)];
    if(!location.owned) return result;
    std::int64_t demand=0,computeMicro=0;
    for(const auto& slot:location.slots) if(slot.chip>=0) {
        const auto& chip=catalog_.chips[static_cast<std::size_t>(slot.chip)];
        demand+=PowerMicroWatts(chip,slot);
        computeMicro+=Scale(static_cast<std::int64_t>(chip.computeMilli)*slot.overclockMilli,catalog_.chassis[static_cast<std::size_t>(slot.chassis)].computeBps,10000);
        result.maintenance+=chip.maintenance;
    }
    const auto supply=std::min(demand,static_cast<std::int64_t>(definition.powerWatts)*1000000);
    // The ratio fits after splitting; normal garage mounting prohibits overload.
    if(demand>0 && supply<demand) computeMicro=Scale(computeMicro,supply/1000,demand/1000);
    result.computeMilli=computeMicro/1000;
    result.suppliedWatts=supply/1000000;
    result.rent=definition.rent;
    result.electricity=Scale(supply,catalog_.electricityPerKwh,1000000000);
    const auto day=(state_.now/Hour/24)%360;
    const int season=(day>=80 && day<170) ? 11500 : (day>=260 ? 9000 : 10000);
    const int region=catalog_.RegionIndex(definition.region);
    result.electricity=Scale(result.electricity,catalog_.regions[static_cast<std::size_t>(region)].tariffBps,10000);
    result.electricity=Scale(result.electricity,season,10000);
    if(state_.nuclearOwned) result.electricity=Scale(result.electricity,catalog_.extensions.nuclearTariffBps,10000);
    return result;
}
Rates Simulation::Economy() const {
    Rates total;
    for(std::size_t i=0;i<state_.locations.size();++i) {
        const auto local=LocationEconomy(static_cast<int>(i));
        total.electricity+=local.electricity; total.maintenance+=local.maintenance; total.rent+=local.rent;
        total.computeMilli+=local.computeMilli; total.suppliedWatts+=local.suppliedWatts;
    }
    // Default online flagship at default token price, no training/portfolio claim.
    // Users are a state variable: installing compute never directly creates money.
    total.revenue=Scale(state_.usersMicro,16,10);
    return total;
}
int Simulation::CourtRiskPpm() const {
    if(!state_.dirtyHistory) return 0;
    int regionBps=10000;
    for(std::size_t i=0;i<state_.locations.size();++i) if(state_.locations[i].owned) {
        const int r=catalog_.RegionIndex(catalog_.locations[i].region);
        regionBps=std::max(regionBps,catalog_.regions[static_cast<std::size_t>(r)].courtBps);
    }
    return 50000*regionBps/10000; // One company risk; selecting a region does not duplicate it.
}
void Simulation::DailyStep() {
    const auto capacityMicro=Economy().computeMilli*1100*(100+state_.reputation);
    state_.usersMicro+=(capacityMicro-state_.usersMicro)/2; // 0.5 daily drift; micro-user precision.
    const int risk=CourtRiskPpm();
    if(risk>0 && Roll(state_.eventRng)<static_cast<std::uint32_t>(risk)) {
        state_.cash-=Dollars(35000); state_.expenses+=Dollars(35000);
        state_.reputation=std::max(0,state_.reputation-20); Notice("Illegal-data court case: fine and reputation loss");
    }
    for(std::size_t i=0;i<state_.locations.size();++i) {
        auto& location=state_.locations[i];
        if(!location.owned) continue;
        for(std::size_t j=0;j<location.slots.size();++j) {
            const auto& slot=location.slots[j]; if(slot.chip<0) continue;
            const auto heat=1000+std::max(0,slot.overclockMilli-1000)*2;
            const auto chance=static_cast<std::int64_t>(catalog_.failurePpm)*catalog_.chassis[static_cast<std::size_t>(slot.chassis)].failureBps*heat/10000000;
            if(Roll(state_.equipmentRng)<static_cast<std::uint32_t>(chance)) FailChip(location.id,static_cast<int>(j));
        }
    }
}
Result Simulation::StartAuction(const std::string& id) {
    auto permitted=BuyBlocked(); if(!permitted.ok) return permitted;
    const auto& x=catalog_.extensions;
    if(!x.auctionConfigured) return Result::Error("BALANCE_TUNABLE: configure auction lot, reserve and rival budget");
    if(state_.auction.phase==AuctionPhase::Open || LocationStatus(id)!=Status::Owned || state_.orders.size()>196) return Result::Error("Auction unavailable or no delivery capacity");
    Auction a; a.phase=AuctionPhase::Open; a.location=id; a.rival=x.rivalName;
    a.chassis=x.auctionRack; a.chip=x.auctionChip; a.quantity=x.auctionQty; a.channel=x.auctionChannel;
    a.reserve=x.auctionReserve; a.increment=x.bidIncrement; a.rivalBudget=x.rivalBudget;
    a.closes=state_.now+x.auctionDuration; a.interval=x.rivalInterval; a.rivalAt=state_.now+x.rivalInterval; a.aggressionPpm=x.rivalAggressionPpm;
    state_.auction=std::move(a); Notice("Auction opened; bids use escrow and two delivery slots are reserved"); return Result::Success();
}
Result Simulation::Bid(Money amount) {
    auto permitted=BuyBlocked(); if(!permitted.ok) return permitted;
    auto& a=state_.auction;
    if(a.phase!=AuctionPhase::Open || state_.now>=a.closes) return Result::Error("Auction is closed");
    const Money minimum=a.currentBid==0 ? a.reserve : a.currentBid+a.increment;
    if(amount<minimum || amount>MoneyLimit || amount>state_.cash+a.escrow) return Result::Error("Bid below minimum or above available budget");
    state_.cash+=a.escrow-amount; a.escrow=amount; a.currentBid=amount; a.playerLeading=true;
    return Result::Success("Bid reserved, not charged twice");
}
void Simulation::AuctionStep() {
    auto& a=state_.auction;
    if(a.phase!=AuctionPhase::Open) return;
    if(state_.now>=a.closes) {
        if(a.playerLeading) {
            state_.capex+=a.escrow;
            // The auction price is the whole lot price. Reuse normal delivery,
            // warehouse, provenance and installation; never call retail checkout again.
            AppendOrder(a.location,Kind::Chassis,a.chassis,a.channel,a.quantity,-1,a.escrow);
            AppendOrder(a.location,Kind::Chip,a.chip,a.channel,a.quantity,-1,0);
            a.escrow=0; a.phase=AuctionPhase::Won; Notice("Auction won; paid lot enters ordinary delivery");
        } else {state_.cash+=a.escrow; a.escrow=0; a.phase=AuctionPhase::Lost; Notice("Auction lost; no purchase charged");}
        return;
    }
    if(state_.now>=a.rivalAt) {
        a.rivalAt+=a.interval;
        const Money next=a.currentBid==0 ? a.reserve : a.currentBid+a.increment;
        if((a.playerLeading || a.currentBid==0) && next<=a.rivalBudget && Roll(state_.auctionRng)<static_cast<std::uint32_t>(a.aggressionPpm)) {
            state_.cash+=a.escrow; a.escrow=0; a.currentBid=next; a.playerLeading=false;
            Notice(a.rival+" bid "+FormatMoney(next)+"; player escrow refunded");
        }
    }
}
Money Simulation::Accrue(Money rate,Tick elapsed,Money& carry) {
    const Money numerator=(rate%Hour)*elapsed+carry;
    const Money posted=(rate/Hour)*elapsed+numerator/Hour;
    carry=numerator%Hour; return posted;
}
Result Simulation::AdvanceReal(Tick microseconds) {
    if(microseconds<0 || microseconds>366LL*24*Hour) return Result::Error("Invalid real-time delta");
    if(state_.paused || state_.ended || microseconds==0) return Result::Success();
    const Tick delta=microseconds*state_.speed;
    if(state_.now>TimeLimit-delta) return Result::Error("Simulation time limit");
    State backup=state_;
    const Tick target=state_.now+delta;
    while(state_.now<target) {
        const Tick day=((state_.now+8*Hour)/(24*Hour)+1)*(24*Hour)-8*Hour;
        Tick until=std::min({target,day,(state_.now/Hour+1)*Hour});
        for(const auto& order:state_.orders) if(order.arrives>state_.now) until=std::min(until,order.arrives);
        if(state_.auction.phase==AuctionPhase::Open) until=std::min({until,state_.auction.closes,state_.auction.rivalAt});
        if(until<=state_.now) {state_=std::move(backup); return Result::Error("Invalid pending time boundary");}
        const auto rates=Economy();
        const Money revenue=Accrue(rates.revenue,until-state_.now,state_.revenueCarry);
        const Money expenses=Accrue(rates.Expenses(),until-state_.now,state_.expenseCarry);
        if(state_.revenue>MoneyLimit-revenue || state_.expenses>MoneyLimit-expenses || state_.cash+revenue-expenses>MoneyLimit || state_.cash+revenue-expenses < -MoneyLimit) {state_=std::move(backup);return Result::Error("Accounting limit");}
        state_.cash+=revenue-expenses; state_.revenue+=revenue; state_.expenses+=expenses; state_.now=until;
        Deliver(); AuctionStep(); if(until==day) DailyStep();
    }
    std::string error;
    if(!Validate(state_,error)) {state_=std::move(backup); return Result::Error(error);}
    return Result::Success();
}
Result Simulation::SetSpeed(int speed) {
    if(speed!=1 && speed!=3) return Result::Error("Supported speeds: 1x and 3x");
    state_.speed=speed; return Result::Success();
}
bool Simulation::Proximity(bool inside,Tick duration,Tick cooldown) {
    if(state_.paused || state_.ended) return false;
    const bool triggered=UpdateProximity(state_.npc,inside,state_.now,duration,cooldown);
    if(triggered) Notice("Phone call near Garage: the equipment delivery is on its way.");
    return triggered;
}
Result Simulation::Restore(const State& candidate) {
    std::string error; if(!Validate(candidate,error)) return Result::Error(error);
    state_=candidate; return Result::Success();
}
bool Simulation::Validate(const State& s,std::string& error) const {
    const auto bad=[&error](const char* text){error=text;return false;};
    if(s.cash < -MoneyLimit || s.cash>MoneyLimit || s.capex<0 || s.capex>MoneyLimit || s.expenses<0 || s.expenses>MoneyLimit || s.revenue<0 || s.revenue>MoneyLimit || s.usersMicro<0 || s.usersMicro>100000000000000LL) return bad("Invalid accounting");
    if(s.now<0 || s.now>TimeLimit || s.restrictedUntil<0 || s.restrictedUntil>TimeLimit || (s.speed!=1 && s.speed!=3) || s.reputation<0 || s.reputation>100 || s.expenseCarry<0 || s.expenseCarry>=Hour || s.revenueCarry<0 || s.revenueCarry>=Hour || !s.equipmentRng || !s.auctionRng || !s.eventRng) return bad("Invalid clock or RNG");
    if(s.locations.size()!=catalog_.locations.size() || s.orders.size()>198 || s.notices.size()>32 || s.orderSequence<0 || s.orderSequence>1000000000LL || s.unlockedRegions.size()>catalog_.regions.size()) return bad("Invalid collection size");
    if(!Has(s.unlockedRegions,"home") || !Has(s.unlockedRegions,s.activeRegion)) return bad("Invalid active region");
    std::set<std::string> regions;
    for(const auto& id:s.unlockedRegions) {const int r=catalog_.RegionIndex(id); if(r<0 || !catalog_.regions[static_cast<std::size_t>(r)].configured || !regions.insert(id).second) return bad("Invalid unlocked region");}
    if(s.nuclearOwned && !catalog_.extensions.nuclearConfigured) return bad("Nuclear balance unavailable");
    for(std::size_t i=0;i<s.locations.size();++i) {
        const auto& l=s.locations[i]; const auto& d=catalog_.locations[i];
        if(l.id!=d.id || l.slots.size()!=static_cast<std::size_t>(d.rows*d.cols) || l.serverSequence<0 || l.serverSequence>1000000000LL || (l.owned && (!d.configured || !Has(s.unlockedRegions,d.region)))) return bad("Invalid location state");
        const auto validStock=[](const Stock& v){return v.total>=0 && v.total<=1000000 && v.grey>=0 && v.grey<=v.total;};
        for(const auto& v:l.chassisStock) if(!validStock(v) || (!l.owned && v.total)) return bad("Invalid chassis provenance");
        for(const auto& v:l.chipStock) if(!validStock(v) || (!l.owned && v.total)) return bad("Invalid chip provenance");
        std::set<std::int64_t> servers;
        for(const auto& slot:l.slots) {
            if(slot.chassis < -1 || slot.chassis>2 || slot.chip < -1 || slot.chip>3 || slot.overclockMilli<500 || slot.overclockMilli>1500 || slot.serverId<0 || slot.serverId>l.serverSequence) return bad("Invalid slot");
            if(slot.chip>=0 && (slot.chassis<0 || slot.chip>catalog_.chassis[static_cast<std::size_t>(slot.chassis)].maxChip || slot.serverId==0 || !servers.insert(slot.serverId).second)) return bad("Invalid installed server");
            if((slot.chip<0 && slot.serverId!=0) || (!l.owned && slot.chassis>=0)) return bad("Invalid empty slot");
        }
    }
    std::set<std::int64_t> orders;
    for(const auto& o:s.orders) {
        const int i=catalog_.LocationIndex(o.location);
        if(i<0 || !s.locations[static_cast<std::size_t>(i)].owned || o.id<=0 || o.id>s.orderSequence || !orders.insert(o.id).second || !ValidKind(o.kind) || !ValidChannel(o.channel) || o.item<0 || o.item>=(o.kind==Kind::Chassis ? 3 : 4) || o.quantity<1 || o.quantity>24 || o.paid<0 || o.paid>MoneyLimit || o.arrives<=s.now || o.arrives>s.now+48*Hour || o.targetCell < -1 || o.targetCell>=static_cast<int>(s.locations[static_cast<std::size_t>(i)].slots.size())) return bad("Invalid pending order");
    }
    for(const auto& text:s.notices) if(text.size()>2048) return bad("Notice too large");
    const auto& a=s.auction;
    if(static_cast<int>(a.phase)<0 || static_cast<int>(a.phase)>3 || a.escrow<0 || a.escrow>MoneyLimit || a.currentBid<0 || a.currentBid>MoneyLimit) return bad("Invalid auction");
    if(a.phase==AuctionPhase::Open) {
        const int l=catalog_.LocationIndex(a.location);
        if(!catalog_.extensions.auctionConfigured || l<0 || !s.locations[static_cast<std::size_t>(l)].owned || s.orders.size()>196 || a.closes<=s.now || a.closes>s.now+24*Hour || a.rivalAt<=s.now || a.interval<=0 || a.interval>24*Hour || a.reserve<=0 || a.reserve>MoneyLimit || a.increment<=0 || a.increment>MoneyLimit || a.rivalBudget<0 || a.rivalBudget>MoneyLimit || a.aggressionPpm<0 || a.aggressionPpm>1000000 || a.chassis<0 || a.chassis>2 || a.chip<0 || a.chip>3 || a.chip>catalog_.chassis[static_cast<std::size_t>(a.chassis)].maxChip || a.quantity<1 || a.quantity>24 || !ValidChannel(a.channel) || (a.playerLeading ? a.escrow!=a.currentBid || a.escrow==0 : a.escrow!=0)) return bad("Invalid active auction");
    } else if(a.escrow!=0) return bad("Closed auction retains escrow");
    if(static_cast<int>(s.npc.mode)<0 || static_cast<int>(s.npc.mode)>2 || s.npc.stageEnds<0 || s.npc.stageEnds>TimeLimit || s.npc.cooldownEnds<0 || s.npc.cooldownEnds>TimeLimit || s.npc.eventCount<0 || s.npc.eventCount>1000000000) return bad("Invalid NPC state");
    return true;
}
} // namespace mai
