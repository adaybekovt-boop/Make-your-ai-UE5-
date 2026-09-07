#include "Campaign/MaiCampaign.h"
#include <algorithm>

namespace mai {
namespace {
std::int64_t Work(std::int64_t rate,Tick elapsed,std::int64_t& carry) {
    const auto remainder=(rate%Hour)*elapsed+carry;
    const auto result=(rate/Hour)*elapsed+remainder/Hour;carry=remainder%Hour;return result;
}
}
void Campaign::Step(Tick elapsed) {
    // One chronological owner advances deliveries, operating costs, review and
    // training. AI review reserves compute instead of granting a second pool.
    const auto trainingComputeForStep=AvailableTrainingCompute();
    for(auto& human:state_.specialists) {
        ReviewSession* active=nullptr;
        for(auto& r:state_.reviews) if(r.method==ReviewMethod::Human && r.phase==ReviewPhase::Active && r.specialistId==human.id) {active=&r;break;}
        if(!active) for(auto& r:state_.reviews) if(r.method==ReviewMethod::Human && r.phase==ReviewPhase::Queued) {r.phase=ReviewPhase::Active;r.specialistId=human.id;r.startedAt=core_.View().now-elapsed;active=&r;break;}
        if(!active) continue;
        active->elapsed+=elapsed;
        const auto rate=static_cast<std::int64_t>(human.volumePerHour)*1000000*(state_.overwork?3:2)/2;
        active->remainingMicro=std::max<std::int64_t>(0,active->remainingMicro-Work(rate,elapsed,active->carry));
        if(active->remainingMicro==0) {
            const int accuracy=std::max(5000,human.accuracyBps-human.fatigueBps/3);
            const bool error=Simulation::Roll(state_.reviewRng)>=static_cast<std::uint32_t>(accuracy*100);
            const auto* batch=Batch(active->batchId);
            const int score=std::clamp(batch->original.scoreBps+(error?-3000:500)-human.fatigueBps/5,0,9800);
            FinishReview(*active,score);human.fatigueBps=std::min(8000,human.fatigueBps+(state_.overwork?2500:1000));
        }
    }
    for(auto& r:state_.reviews) if(r.method==ReviewMethod::Manual && r.phase==ReviewPhase::Active) r.elapsed+=elapsed;
    for(auto& r:state_.reviews) if(r.method==ReviewMethod::AI && r.phase!=ReviewPhase::Complete) {
        // A single AI reviewer has exactly one active queue slot.
        const auto compute=std::min<std::int64_t>(state_.ai.computeMilli,core_.Economy().computeMilli);
        if(compute<=0) break;
        if(r.phase==ReviewPhase::Queued) {r.phase=ReviewPhase::Active;r.startedAt=core_.View().now-elapsed;}
        r.elapsed+=elapsed;
        const auto rate=compute*rules_.aiVolumePerComputeHour*1000;
        r.remainingMicro=std::max<std::int64_t>(0,r.remainingMicro-Work(rate,elapsed,r.carry));
        if(r.remainingMicro==0) {
            auto* b=MutableBatch(r.batchId);
            // Shared category bias, not an independent perfect answer per item.
            const int bias=(b->type==DataType::Image || b->id%3==0)?state_.ai.biasBps:state_.ai.biasBps/3;
            const bool error=Simulation::Roll(state_.reviewRng)>=static_cast<std::uint32_t>(std::max(1000,state_.ai.accuracyBps-bias)*100);
            const int score=std::clamp(b->original.scoreBps+300-bias-(error?2300:0),0,9700);
            FinishReview(r,score);++state_.ai.reviewed;
        }
        break;
    }
    for(auto& t:state_.training) if(t.phase==JobPhase::Running) {
        const auto compute=std::min<std::int64_t>(1000000000,trainingComputeForStep);
        if(compute<=0) break;
        const auto rate=compute*rules_.volumePerComputeHour*1000*Difficulty()->trainingBps/10000;
        t.elapsed+=elapsed;t.remainingMicro=std::max<std::int64_t>(0,t.remainingMicro-Work(rate,elapsed,t.carry));
        if(t.remainingMicro==0) {
            t.phase=JobPhase::Complete;auto* b=MutableBatch(t.batchId);b->status=DatasetStatus::Trained;
            state_.modelMicroIQ=std::min<std::int64_t>(1000000000000LL,state_.modelMicroIQ+t.expectedGainMicroIQ);
            if(b->quality.legalBps>0) {
                state_.legalExposureBps=std::min(10000,std::max(state_.legalExposureBps,b->quality.legalBps*Difficulty()->legalBps/10000));
                auto s=core_.View();s.dirtyHistory=true;core_.Restore(s);
            }
            Record("training-complete",std::to_string(t.id)+" gainMicroIQ="+std::to_string(t.expectedGainMicroIQ));
        }
        break;
    }
    const auto day=(core_.View().now+8*Hour)/(24*Hour);
    if(day>state_.lastLegalDay) {
        state_.lastLegalDay=day;
        for(auto& h:state_.specialists) if(!state_.overwork) h.fatigueBps=std::max(0,h.fatigueBps-1000);
        if(!state_.specialists.empty()) state_.employeeCareBps=std::clamp(state_.employeeCareBps+(state_.overwork?-1000:200),0,10000);
        if(core_.View().dirtyHistory && Simulation::Roll(state_.legalRng)<static_cast<std::uint32_t>(core_.CourtRiskPpm())) {
            ++state_.warnings;state_.legalExposureBps=std::min(10000,state_.legalExposureBps+1500);
            Record("legal-warning","Shared source court-risk calculation; review does not legalize an unlicensed dataset");
        }
    }
    if(core_.View().cash<=0) {if(state_.insolventSince<0) state_.insolventSince=core_.View().now;}
    else state_.insolventSince=-1;
    EvaluateEnding();
}
Result Campaign::AdvanceReal(Tick microseconds) {
    if(microseconds<0 || microseconds>168*Hour) return Result::Error("Invalid campaign time delta");
    if(!CanPlay() || core_.View().paused || microseconds==0) return Result::Success();
    const auto beforeCore=core_;const auto beforeState=state_;
    const int speed=core_.View().speed;
    // Fixed game-time quantum: 1/60 game hour. Remainder is saved, so frame
    // splitting, pause and restart cannot mint review work or training progress.
    constexpr Tick quantum=Hour/60;
    state_.fixedCarry+=microseconds*speed;
    while(state_.fixedCarry>=quantum) {
        state_.fixedCarry-=quantum;core_.SetSpeed(1);auto result=core_.AdvanceReal(quantum);core_.SetSpeed(speed);
        if(!result.ok) {core_=beforeCore;state_=beforeState;return result;}
        Step(quantum);
        if(core_.View().ended) {state_.fixedCarry=0;break;}
    }
    std::string error;if(!Validate(error)) {core_=beforeCore;state_=beforeState;return Result::Error(error);}
    return Result::Success();
}
} // namespace mai
