#pragma once

#include <cstdint>

namespace inverter
{
enum class OperatingState : std::uint8_t
{
    Standby,
    Precharging,
    MainContactorClosing,
    Ready,
    Driving,
    FaultLatched
};

enum class FaultCode : std::uint8_t
{
    None,
    EmergencyStop,
    BatteryUndervoltage,
    PrechargeTimeout,
    DcLinkUndervoltage,
    InverterOvertemperature,
    PhaseOvercurrent
};

// Values sampled by the hardware abstraction layer before the periodic task runs.
struct Inputs
{
    bool ignitionOn{false};
    bool driveRequest{false};
    bool faultResetRequest{false};
    bool emergencyStopActive{false};
    float batteryVoltageV{0.0F};
    float dcLinkVoltageV{0.0F};
    float inverterTemperatureC{25.0F};
    float phaseCurrentA{0.0F};
    float torqueRequestNm{0.0F};
};

// Commands written to the hardware abstraction layer after the task completes.
struct Outputs
{
    bool prechargeRelayClosed{false};
    bool mainContactorClosed{false};
    bool pwmEnabled{false};
    bool faultLampOn{false};
    float torqueCommandNm{0.0F};
};

class InverterController final
{
public:
    InverterController() noexcept;
    void tick(const Inputs& inputs, std::uint32_t elapsedMs) noexcept;

    [[nodiscard]] OperatingState state() const noexcept;
    [[nodiscard]] FaultCode fault() const noexcept;
    [[nodiscard]] const Outputs& outputs() const noexcept;

private:
    class State;
    class StandbyState;
    class PrechargingState;
    class MainContactorClosingState;
    class ReadyState;
    class DrivingState;
    class FaultLatchedState;

    static const State& standbyState() noexcept;
    static const State& prechargingState() noexcept;
    static const State& mainContactorClosingState() noexcept;
    static const State& readyState() noexcept;
    static const State& drivingState() noexcept;
    static const State& faultLatchedState() noexcept;

    void transitionTo(const State& next) noexcept;
    void trip(FaultCode code) noexcept;
    void setSafeOutputs(bool faultLampOn) noexcept;

    const State* currentState_;
    Outputs outputs_{};
    FaultCode latchedFault_{FaultCode::None};
    std::uint32_t timeInStateMs_{0U};
};

[[nodiscard]] const char* toString(OperatingState state) noexcept;
[[nodiscard]] const char* toString(FaultCode fault) noexcept;
} // namespace inverter

