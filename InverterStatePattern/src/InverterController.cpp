#include "firmware/InverterController.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace inverter
{
namespace
{
constexpr float kMinimumBatteryVoltageV = 250.0F;
constexpr float kPrechargeCompleteRatio = 0.90F;
constexpr float kMinimumEnergizedRatio = 0.75F;
constexpr float kMaximumTemperatureC = 100.0F;
constexpr float kResetTemperatureC = 80.0F;
constexpr float kMaximumPhaseCurrentA = 450.0F;
constexpr float kResetMaximumCurrentA = 5.0F;
constexpr float kMaximumTorqueNm = 220.0F;
constexpr std::uint32_t kPrechargeTimeoutMs = 1'500U;
constexpr std::uint32_t kMainContactorSettleMs = 100U;

bool dcLinkIsAboveRatio(const Inputs& inputs, const float ratio) noexcept
{
    return inputs.dcLinkVoltageV >= (inputs.batteryVoltageV * ratio);
}
} // namespace

// State objects contain behavior only; their data lives in the controller context.
// One static instance per state avoids dynamic allocation.
class InverterController::State
{
public:
    virtual ~State() = default;
    [[nodiscard]] virtual OperatingState id() const noexcept = 0;
    virtual void enter(InverterController& context) const noexcept = 0;
    virtual void update(InverterController& context, const Inputs& inputs) const noexcept = 0;
};

class InverterController::StandbyState final : public InverterController::State
{
public:
    OperatingState id() const noexcept override { return OperatingState::Standby; }
    void enter(InverterController& context) const noexcept override
    {
        context.setSafeOutputs(false);
    }
    void update(InverterController& context, const Inputs& inputs) const noexcept override
    {
        if (!inputs.ignitionOn) return;
        if (inputs.batteryVoltageV < kMinimumBatteryVoltageV)
        {
            context.trip(FaultCode::BatteryUndervoltage);
            return;
        }
        context.transitionTo(InverterController::prechargingState());
    }
};

class InverterController::PrechargingState final : public InverterController::State
{
public:
    OperatingState id() const noexcept override { return OperatingState::Precharging; }
    void enter(InverterController& context) const noexcept override
    {
        context.setSafeOutputs(false);
        context.outputs_.prechargeRelayClosed = true;
    }
    void update(InverterController& context, const Inputs& inputs) const noexcept override
    {
        if (!inputs.ignitionOn)
        {
            context.transitionTo(InverterController::standbyState());
            return;
        }
        if (inputs.batteryVoltageV < kMinimumBatteryVoltageV)
        {
            context.trip(FaultCode::BatteryUndervoltage);
            return;
        }
        if (dcLinkIsAboveRatio(inputs, kPrechargeCompleteRatio))
        {
            context.transitionTo(InverterController::mainContactorClosingState());
            return;
        }
        // Do not keep heating the precharge resistor when the DC link does not rise.
        if (context.timeInStateMs_ >= kPrechargeTimeoutMs)
        {
            context.trip(FaultCode::PrechargeTimeout);
        }
    }
};

class InverterController::MainContactorClosingState final : public InverterController::State
{
public:
    OperatingState id() const noexcept override
    {
        return OperatingState::MainContactorClosing;
    }
    void enter(InverterController& context) const noexcept override
    {
        context.setSafeOutputs(false);
        // Keep precharge closed during the main contactor's mechanical pull-in time.
        context.outputs_.prechargeRelayClosed = true;
        context.outputs_.mainContactorClosed = true;
    }
    void update(InverterController& context, const Inputs& inputs) const noexcept override
    {
        if (!inputs.ignitionOn)
        {
            context.transitionTo(InverterController::standbyState());
            return;
        }
        if (!dcLinkIsAboveRatio(inputs, kMinimumEnergizedRatio))
        {
            context.trip(FaultCode::DcLinkUndervoltage);
            return;
        }
        if (context.timeInStateMs_ >= kMainContactorSettleMs)
        {
            context.transitionTo(InverterController::readyState());
        }
    }
};

class InverterController::ReadyState final : public InverterController::State
{
public:
    OperatingState id() const noexcept override { return OperatingState::Ready; }
    void enter(InverterController& context) const noexcept override
    {
        context.setSafeOutputs(false);
        context.outputs_.mainContactorClosed = true;
    }
    void update(InverterController& context, const Inputs& inputs) const noexcept override
    {
        if (!inputs.ignitionOn)
        {
            context.transitionTo(InverterController::standbyState());
            return;
        }
        if (!dcLinkIsAboveRatio(inputs, kMinimumEnergizedRatio))
        {
            context.trip(FaultCode::DcLinkUndervoltage);
            return;
        }
        if (inputs.driveRequest)
        {
            context.transitionTo(InverterController::drivingState());
        }
    }
};

class InverterController::DrivingState final : public InverterController::State
{
public:
    OperatingState id() const noexcept override { return OperatingState::Driving; }
    void enter(InverterController& context) const noexcept override
    {
        context.setSafeOutputs(false);
        context.outputs_.mainContactorClosed = true;
        context.outputs_.pwmEnabled = true;
    }
    void update(InverterController& context, const Inputs& inputs) const noexcept override
    {
        if (!inputs.ignitionOn)
        {
            context.transitionTo(InverterController::standbyState());
            return;
        }
        if (!inputs.driveRequest)
        {
            context.transitionTo(InverterController::readyState());
            return;
        }
        if (!dcLinkIsAboveRatio(inputs, kMinimumEnergizedRatio))
        {
            context.trip(FaultCode::DcLinkUndervoltage);
            return;
        }
        // A faster current loop would apply its own slew-rate and current limits.
        context.outputs_.torqueCommandNm =
            std::clamp(inputs.torqueRequestNm, -kMaximumTorqueNm, kMaximumTorqueNm);
    }
};

class InverterController::FaultLatchedState final : public InverterController::State
{
public:
    OperatingState id() const noexcept override { return OperatingState::FaultLatched; }
    void enter(InverterController& context) const noexcept override
    {
        // Apply safe outputs as part of the transition, not one tick later.
        context.setSafeOutputs(true);
    }
    void update(InverterController& context, const Inputs& inputs) const noexcept override
    {
        const bool resetConditionsAreSafe =
            !inputs.ignitionOn && inputs.faultResetRequest &&
            !inputs.emergencyStopActive &&
            inputs.inverterTemperatureC <= kResetTemperatureC &&
            std::fabs(inputs.phaseCurrentA) <= kResetMaximumCurrentA;
        if (resetConditionsAreSafe)
        {
            context.latchedFault_ = FaultCode::None;
            context.transitionTo(InverterController::standbyState());
        }
    }
};

InverterController::InverterController() noexcept : currentState_(&standbyState())
{
    currentState_->enter(*this);
}

void InverterController::tick(const Inputs& inputs, const std::uint32_t elapsedMs) noexcept
{
    const auto remaining = std::numeric_limits<std::uint32_t>::max() - timeInStateMs_;
    timeInStateMs_ += std::min(elapsedMs, remaining);

    if (state() != OperatingState::FaultLatched)
    {
        // Cross-cutting protection stays in the context so no state can omit it.
        if (inputs.emergencyStopActive)
        {
            trip(FaultCode::EmergencyStop);
            return;
        }
        if (inputs.inverterTemperatureC > kMaximumTemperatureC)
        {
            trip(FaultCode::InverterOvertemperature);
            return;
        }
        if (std::fabs(inputs.phaseCurrentA) > kMaximumPhaseCurrentA)
        {
            trip(FaultCode::PhaseOvercurrent);
            return;
        }
    }
    currentState_->update(*this, inputs);
}

OperatingState InverterController::state() const noexcept { return currentState_->id(); }
FaultCode InverterController::fault() const noexcept { return latchedFault_; }
const Outputs& InverterController::outputs() const noexcept { return outputs_; }

void InverterController::transitionTo(const State& next) noexcept
{
    currentState_ = &next;
    timeInStateMs_ = 0U;
    currentState_->enter(*this);
}
void InverterController::trip(const FaultCode code) noexcept
{
    latchedFault_ = code;
    transitionTo(faultLatchedState());
}
void InverterController::setSafeOutputs(const bool faultLampOn) noexcept
{
    outputs_ = {};
    outputs_.faultLampOn = faultLampOn;
}

const InverterController::State& InverterController::standbyState() noexcept
{
    static const StandbyState value;
    return value;
}
const InverterController::State& InverterController::prechargingState() noexcept
{
    static const PrechargingState value;
    return value;
}
const InverterController::State& InverterController::mainContactorClosingState() noexcept
{
    static const MainContactorClosingState value;
    return value;
}
const InverterController::State& InverterController::readyState() noexcept
{
    static const ReadyState value;
    return value;
}
const InverterController::State& InverterController::drivingState() noexcept
{
    static const DrivingState value;
    return value;
}
const InverterController::State& InverterController::faultLatchedState() noexcept
{
    static const FaultLatchedState value;
    return value;
}

const char* toString(const OperatingState value) noexcept
{
    switch (value)
    {
    case OperatingState::Standby: return "Standby";
    case OperatingState::Precharging: return "Precharging";
    case OperatingState::MainContactorClosing: return "MainContactorClosing";
    case OperatingState::Ready: return "Ready";
    case OperatingState::Driving: return "Driving";
    case OperatingState::FaultLatched: return "FaultLatched";
    }
    return "UnknownState";
}
const char* toString(const FaultCode value) noexcept
{
    switch (value)
    {
    case FaultCode::None: return "None";
    case FaultCode::EmergencyStop: return "EmergencyStop";
    case FaultCode::BatteryUndervoltage: return "BatteryUndervoltage";
    case FaultCode::PrechargeTimeout: return "PrechargeTimeout";
    case FaultCode::DcLinkUndervoltage: return "DcLinkUndervoltage";
    case FaultCode::InverterOvertemperature: return "InverterOvertemperature";
    case FaultCode::PhaseOvercurrent: return "PhaseOvercurrent";
    }
    return "UnknownFault";
}
} // namespace inverter

