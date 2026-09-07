#pragma once
#include "Core/MaiDomain.h"
#include <optional>

// Portable production domain, used directly by UE. New review/difficulty/ending
// tuning is deliberately separate from the unchanged browser-derived catalog.
namespace mai {
enum class Screen : int { Loading, MainMenu, NewGame, Difficulty, Prologue, CityMap, Gameplay, Training, Ending, Results, Settings };
enum class LoadPhase : int { Idle, Loading, Ready, Failed };
enum class DataType : int { Text, Image, Mixed };
enum class DatasetStatus : int { Purchased, Unreviewed, Reviewing, Verified, Rejected, Training, Trained };
enum class ReviewMethod : int { None, Manual, Human, AI };
enum class ReviewPhase : int { Queued, Active, Complete };
enum class JobPhase : int { Running, Paused, Complete };
enum class EndingKind : int { None, Acquisition, Independent, OpenModel, Bankruptcy, Regulator };
enum class ReviewChoice : int { Left = 0, Right = 1, BothBad = 2 };

struct SessionSettings { int textScaleBps = 10000; bool reducedMotion = false; };
struct WalkState { int xCm = 0, yCm = 0, zCm = 90, facingDeg = 0; };
struct DifficultyProfile {
    std::string id = "Normal";
    int capitalBps = 10000, procurementBps = 10000, defectBps = 10000, failureBps = 10000;
    int legalBps = 10000, hiringBps = 10000, competitorBps = 10000, trainingBps = 10000;
    Tick insolvencyGrace = 48 * Hour;
};
struct DatasetQuality { int scoreBps = 0, noiseBps = 0, legalBps = 0, acceptedBps = 10000; };
struct DatasetItem {
    std::string id, prompt, left, right;
    DataType type = DataType::Text;
    int betterSide = 0;
    // Optional authored texture slots. Empty slots are visibly captioned cards,
    // not claims that photos have been generated/imported.
    std::string leftAsset, rightAsset;
};
struct DatasetOffer {
    std::string id, name;
    DataType type = DataType::Text;
    int volume = 10;
    Money cost = Dollars(1400);
    DatasetQuality quality{9500, 500, 0, 10000};
};
struct ReviewDecision { std::string itemId; int side = 0; bool correct = false; Tick at = 0; };
struct DatasetBatch {
    std::int64_t id = 0;
    std::string offerId;
    DataType type = DataType::Text;
    int volume = 0;
    Money cost = 0;
    DatasetQuality original, quality;
    DatasetStatus status = DatasetStatus::Purchased;
    ReviewMethod method = ReviewMethod::None;
    Tick purchasedAt = 0, reviewTime = 0;
    std::int64_t reviewId = 0, trainingId = 0;
};
struct DatasetInventory { std::vector<DatasetBatch> batches; std::int64_t sequence = 0; };
struct ReviewSession {
    std::int64_t id = 0, batchId = 0, specialistId = 0;
    ReviewMethod method = ReviewMethod::None;
    ReviewPhase phase = ReviewPhase::Queued;
    std::vector<DatasetItem> items;
    std::vector<ReviewDecision> decisions;
    Tick startedAt = 0, elapsed = 0;
    std::int64_t remainingMicro = 0, carry = 0;
    int resultQualityBps = 0, reward = 0;
};
struct Specialist {
    std::int64_t id = 0;
    int accuracyBps = 9000, volumePerHour = 25, fatigueBps = 0;
    Money hireCost = 0, batchFee = 0;
};
struct AIReviewer {
    bool created = false;
    int accuracyBps = 8600, biasBps = 1600, level = 1, computeMilli = 500;
    std::int64_t reviewed = 0;
};
struct TrainingJob {
    std::int64_t id = 0, batchId = 0, remainingMicro = 0, carry = 0, expectedGainMicroIQ = 0;
    Tick startedAt = 0, elapsed = 0;
    JobPhase phase = JobPhase::Running;
};
struct DecisionRecord { std::string action, detail; Tick at = 0; };
struct LoadingState {
    LoadPhase phase = LoadPhase::Idle;
    std::uint64_t generation = 0;
    Screen destination = Screen::MainMenu;
    std::string interior, operation, error;
    int progressBps = -1; // -1 is indeterminate, never an invented percentage.
};
struct EndingMetrics {
    Money cash = 0, value = 0, debt = 0, profitPerHour = 0;
    std::int64_t modelMicroIQ = 0;
    int reputation = 0, legalBps = 0, dataQualityBps = 0, dependencyBps = 0;
    int employeeCareBps = 10000, automationBps = 0, successfulReviews = 0, ignoredWarnings = 0;
    bool international = false, saleAccepted = false, saleDeclined = false, openChosen = false, bankrupt = false;
    std::string difficulty;
};
struct EndingProfile {
    EndingKind kind = EndingKind::None;
    int priority = 0;
    std::string id, title, line;
    Money minValue = 0;
    std::int64_t minModelMicroIQ = 0;
    int minReputation = 0, maxLegalBps = 10000, minDataQualityBps = 0, maxDependencyBps = 10000;
    int minEmployeeCareBps = 0, maxAutomationBps = 10000;
    bool allowReturnToSave = true;
    int regulatorMinLegalBps = 9000, regulatorMinIgnoredWarnings = 3;
};
struct EndingResult {
    EndingKind kind = EndingKind::None;
    std::string id, title, line;
    Money deal = 0;
    bool offerOnly = false, allowReturnToSave = false;
    EndingMetrics metrics;
    std::vector<DecisionRecord> decisions;
};
struct FictionalCharacter {
    std::string id = "elon-max", name = "Elon Max";
    std::string biography = "Fictional founder of Maximal Teapots, a company launching luxury kettles into orbit. Not a real person's biography.";
    std::string portraitSlot = "/Game/Scaffold/Portraits/T_ElonMax_Fictional";
    std::string fallback = "ELON MAX / fictional remade portrait pending UE import / author-supplied likeness, not a documentary photo";
    std::string sourcePortrait = "Content/Source/Portraits/elon_max_remade.jpg";
    bool fictional = true;
};
struct CampaignRules {
    // All new values are BALANCE_TUNABLE. Prices/volume and training throughput
    // below are derived from config.ts; official-10 is a new prorated sample.
    std::vector<DifficultyProfile> difficulties;
    std::vector<DatasetOffer> offers;
    std::vector<DatasetItem> items;
    std::vector<EndingProfile> endings;
    FictionalCharacter buyer;
    int manualSteps = 4, verifiedThresholdBps = 6000;
    Money humanHire = Dollars(500), humanBatch = Dollars(60), aiSetup = Dollars(2000), aiUpgrade = Dollars(1000);
    int humanAccuracyBps = 9000, humanVolumePerHour = 25, aiVolumePerComputeHour = 200;
    int volumePerComputeHour = 2, microIQPerVolume = 60000;
    Money remediationCost = Dollars(2000);
    static CampaignRules Defaults();
    bool Valid(std::string& error) const;
    std::uint32_t Fingerprint() const;
};
class EndingEvaluator {
public:
    static EndingResult Evaluate(const EndingMetrics& metrics, const std::vector<EndingProfile>& profiles);
};
struct CampaignState {
    Screen screen = Screen::Loading, firstScreen = Screen::Loading, lastScreen = Screen::Loading, resumeScreen = Screen::MainMenu;
    LoadingState loading;
    std::string difficulty, companyName, interior;
    int prologueStep = 0;
    std::uint32_t seed = 42, reviewRng = 43, legalRng = 44;
    DatasetInventory inventory;
    std::vector<ReviewSession> reviews;
    std::vector<Specialist> specialists;
    AIReviewer ai;
    std::vector<TrainingJob> training;
    std::vector<DecisionRecord> decisions;
    std::int64_t reviewSequence = 0, specialistSequence = 0, trainingSequence = 0, modelMicroIQ = 0;
    Money debt = 0;
    int legalExposureBps = 0, dependencyBps = 0, employeeCareBps = 10000;
    int ignoredWarnings = 0, warnings = 0, reward = 0;
    bool overwork = false, saleAccepted = false, saleDeclined = false, openChosen = false, quitRequested = false;
    Tick insolventSince = -1, fixedCarry = 0, lastLegalDay = 0;
    WalkState walk;
    EndingResult ending;
};
struct InteriorPoint { std::string id, action; int xCm = 0, yCm = 0; };
struct InteriorProfile {
    std::string id, name, type, map;
    bool walkable = false;
    std::vector<InteriorPoint> points;
    std::vector<std::string> systems, equipment;
};
std::vector<InteriorProfile> InteriorProfiles();
bool WithinInteractionRange(int dxCm, int dyCm, int dzCm, int rangeCm);
std::string ScreenName(Screen screen);
std::string DatasetStatusName(DatasetStatus status);

class Campaign {
public:
    explicit Campaign(Catalog base = Catalog::Defaults(), CampaignRules rules = CampaignRules::Defaults(), std::uint32_t seed = 42);
    const CampaignState& View() const { return state_; }
    const CampaignRules& Rules() const { return rules_; }
    Simulation& Core() { return core_; }
    const Simulation& Core() const { return core_; }
    const DifficultyProfile* Difficulty() const;
    bool CanPlay() const;
    Result Reset(std::uint32_t seed);
    Result BeginNewGame();
    Result ShowDifficulty();
    Result ChooseDifficulty(const std::string& id);
    Result PrologueAction(const std::string& input);
    Result BeginLoad(Screen destination, const std::string& interior = {});
    Result ReportLoading(std::uint64_t generation, int progressBps, const std::string& operation);
    Result CompleteLoad(std::uint64_t generation, bool success, const std::string& error = {});
    Result CancelLoad();
    Result RetryLoad();
    Result ShowScreen(Screen screen);
    Result LoadFromMenu(const std::string& encoded);
    Result RequestQuit();
    Result SetTextScale(int textScaleBps);
    Result SetReducedMotion(bool enabled);
    Result BuyDataset(const std::string& offer);
    Result StartReview(std::int64_t batch, ReviewMethod method);
    Result ChooseReview(std::int64_t session, int side);
    Result SkipReview(std::int64_t session);
    Result CancelReview(std::int64_t session);
    Result HireSpecialist();
    Result CreateAIReviewer();
    Result ImproveAIReviewer();
    Result SetOverwork(bool enabled);
    Result StartTraining(std::int64_t batch);
    Result PauseTraining(bool paused);
    Result AdvanceReal(Tick microseconds);
    Result ChooseEnding(EndingKind choice);
    Result DeclineSale();
    Result EvaluateEnding();
    Result AcknowledgeEnding();
    Result Remediate();
    Result IgnoreWarning();
    Result Borrow(Money amount);
    Result Repay(Money amount);
    Result TalkToGarageNpc();
    Result EnterWalk(const std::string& interior);
    Result ReturnToCity();
    Result SetWalkPosition(int xCm, int yCm, int zCm, int facingDeg);
    Result InteractNearby(int rangeCm = 150);
    std::optional<InteriorPoint> NearbyPoint(int rangeCm = 150) const;
    EndingMetrics Metrics() const;
    std::int64_t AvailableTrainingCompute() const;
    const DatasetBatch* Batch(std::int64_t id) const;
    const ReviewSession* Review(std::int64_t id) const;
    const SessionSettings& Settings() const { return settings_; }
    bool WantsQuit() const { return state_.quitRequested; }
    std::string Save() const;
    Result Load(const std::string& encoded);
    bool Validate(std::string& error) const;
private:
    Catalog base_;
    CampaignRules rules_;
    Simulation core_;
    CampaignState state_;
    SessionSettings settings_;
    static Catalog ApplyDifficulty(Catalog base, const DifficultyProfile& profile);
    void Visit(Screen screen);
    void Record(const std::string& action, const std::string& detail);
    Result Spend(Money amount, bool capital = false);
    DatasetBatch* MutableBatch(std::int64_t id);
    void FinishReview(ReviewSession& review, int scoreBps);
    void Step(Tick elapsed);
    Result ReadBody(const std::string& body, int version);
};
} // namespace mai
