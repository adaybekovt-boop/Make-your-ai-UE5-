#pragma once

// Engine-independent authoritative domain. The very same implementation is used
// by the UE subsystems and the native/Automation tests; this is not a test double.
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mai {
using Money = std::int64_t;
using Tick = std::int64_t;
constexpr Money Unit = 1000000; // One dollar, stored as microdollars, never floating cash.
constexpr Tick Hour = 60000000; // One game hour = 60 real seconds at 1x; one tick = 1 real microsecond at 1x.
constexpr Money MoneyLimit = 1000000000000000LL;
constexpr Tick TimeLimit = 36500LL * 24 * Hour;
constexpr int SaveVersion = 1;
constexpr Money Dollars(Money value) { return value * Unit; }

enum class Channel : int { Official, Grey };
enum class Kind : int { Chassis, Chip };
enum class Status : int { Locked, Available, Owned };
enum class AuctionPhase : int { None, Open, Won, Lost };
enum class NpcMode : int { Idle, PhoneCall, React };

struct Result {
    bool ok = false;
    std::string message;
    static Result Success(std::string text = {}) { return {true, std::move(text)}; }
    static Result Error(std::string text) { return {false, std::move(text)}; }
};
struct ChipDef {
    std::string id, name;
    Money price = 0, maintenance = 0;
    int watts = 0, computeMilli = 0;
    std::array<int, 2> deliveryHours{};
};
struct ChassisDef {
    std::string id, name;
    Money price = 0;
    int maxChip = 0, failureBps = 10000, computeBps = 10000;
    std::array<int, 2> deliveryHours{};
};
struct RegionDef {
    std::string id, name;
    Money unlockPrice = 0;
    int tariffBps = 10000, courtBps = 10000;
    bool configured = true, balanceTunable = false;
};
struct LocationDef {
    std::string id, name, region;
    Money price = 0, rent = 0;
    int rows = 0, cols = 0, powerWatts = 0;
    bool configured = true;
};
struct ExtensionRules {
    // No source balance exists for these extensions. Production defaults are
    // deliberately disabled, not free purchases masquerading as final balance.
    bool nuclearConfigured = false, auctionConfigured = false;
    Money nuclearPrice = 0;
    int nuclearTariffBps = 10000;
    std::string rivalName = "Competitor";
    Money auctionReserve = 0, bidIncrement = 0, rivalBudget = 0;
    Tick auctionDuration = 2 * Hour, rivalInterval = Hour / 4;
    int rivalAggressionPpm = 1000000;
    int auctionRack = 0, auctionChip = 0, auctionQty = 1;
    Channel auctionChannel = Channel::Official;
};
struct Catalog {
    std::array<ChipDef, 4> chips;
    std::array<ChassisDef, 3> chassis;
    std::vector<LocationDef> locations;
    std::vector<RegionDef> regions;
    ExtensionRules extensions;
    Money electricityPerKwh = Dollars(18);
    int defectPpm = 80000, failurePpm = 2000;
    static Catalog Defaults();
    bool Valid(std::string& error) const;
    std::uint32_t Fingerprint() const;
    int LocationIndex(const std::string& id) const;
    int RegionIndex(const std::string& id) const;
};
struct Stock { int total = 0, grey = 0; };
struct Slot {
    int chassis = -1, chip = -1, overclockMilli = 1000;
    std::int64_t serverId = 0;
};
struct Location {
    std::string id;
    bool owned = false;
    std::int64_t serverSequence = 0;
    std::array<Stock, 3> chassisStock{};
    std::array<Stock, 4> chipStock{};
    std::vector<Slot> slots;
};
struct Order {
    std::int64_t id = 0;
    std::string location;
    Kind kind = Kind::Chassis;
    Channel channel = Channel::Official;
    int item = 0, quantity = 1, targetCell = -1;
    Money paid = 0;
    Tick arrives = 0;
};
struct Auction {
    AuctionPhase phase = AuctionPhase::None;
    std::string location, rival;
    int chassis = 0, chip = 0, quantity = 1;
    Channel channel = Channel::Official;
    Money currentBid = 0, escrow = 0, increment = 0, rivalBudget = 0, reserve = 0;
    bool playerLeading = false;
    Tick closes = 0, rivalAt = 0, interval = 0;
    int aggressionPpm = 1000000;
};
struct NpcState {
    NpcMode mode = NpcMode::Idle;
    Tick stageEnds = 0, cooldownEnds = 0;
    bool wasInside = false;
    std::int64_t eventCount = 0;
};
// Deterministic state machine: proximity, not camera distance or random Tick events.
bool UpdateProximity(NpcState& state, bool inside, Tick now, Tick duration, Tick cooldown);
struct State {
    Money cash = Dollars(12000), capex = 0, expenses = 0, revenue = 0;
    Money expenseCarry = 0, revenueCarry = 0;
    Tick now = 0, restrictedUntil = 0;
    bool paused = false, ended = false, nuclearOwned = false, dirtyHistory = false;
    int speed = 1, reputation = 50;
    std::int64_t usersMicro = 0, orderSequence = 0;
    std::uint32_t equipmentRng = 1, auctionRng = 2, eventRng = 3;
    std::string activeRegion = "home";
    std::vector<std::string> unlockedRegions{"home"};
    std::vector<Location> locations;
    std::vector<Order> orders;
    Auction auction;
    NpcState npc;
    std::vector<std::string> notices;
};
struct Rates {
    Money electricity = 0, maintenance = 0, rent = 0, revenue = 0;
    std::int64_t computeMilli = 0, suppliedWatts = 0;
    Money Expenses() const { return electricity + maintenance + rent; }
};
class Simulation {
public:
    explicit Simulation(Catalog catalog = Catalog::Defaults(), std::uint32_t seed = 1);
    const Catalog& Definitions() const { return catalog_; }
    const State& View() const { return state_; }
    Result Reset(std::uint32_t seed);
    Result Restore(const State& candidate);
    bool Validate(const State& candidate, std::string& error) const;
    Status LocationStatus(const std::string& id) const;
    Result BuyLocation(const std::string& id);
    Result UnlockRegion(const std::string& id);
    Result SwitchRegion(const std::string& id);
    Result BuyNuclear();
    static Money OrderPrice(Money base, Channel channel, int quantity);
    static int DiscountPercent(int quantity);
    Result OrderItem(const std::string& location, Kind kind, int item, Channel channel, int quantity, int cell = -1);
    Result OrderKit(const std::string& location, int chassis, int chip, Channel channel, int quantity, int cell);
    Result MountChassis(const std::string& location, int cell, int chassis);
    Result MountChip(const std::string& location, int cell, int chip);
    Result SetOverclock(const std::string& location, int cell, int milli);
    Result FailChip(const std::string& location, int cell);
    Result StartAuction(const std::string& location);
    Result Bid(Money bid);
    Result AdvanceReal(Tick microseconds);
    Result SetSpeed(int speed);
    void SetPaused(bool paused) { state_.paused = paused; }
    bool Proximity(bool inside, Tick duration, Tick cooldown);
    Rates Economy() const;
    Rates LocationEconomy(int index) const;
    int CourtRiskPpm() const;
    std::string Save() const;
    Result Load(const std::string& encoded);
    static std::uint32_t Roll(std::uint32_t& seed);
private:
    Catalog catalog_;
    State state_;
    Result BuyBlocked() const;
    Result ValidateOrder(const std::string& location, Kind kind, int item, Channel channel, int quantity, int cell) const;
    int ReservedOrders() const;
    void AppendOrder(const std::string& location, Kind kind, int item, Channel channel, int quantity, int cell, Money paid);
    void Deliver();
    void AuctionStep();
    void DailyStep();
    void Notice(const std::string& text);
    void ChargeFailure(Money price);
    static bool Take(Stock& stock);
    static Money Accrue(Money rate, Tick elapsed, Money& carry);
};
std::string FormatMoney(Money money);
} // namespace mai
