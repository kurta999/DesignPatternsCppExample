#pragma once

/**
 * @file InverterController.hpp
 * @brief Allocation-free inverter supervisor implemented with the State pattern.
 */

#include <cstdint>

namespace inverter
{
/** @addtogroup inverter_state
 *  @{
 */
/** @brief Externally visible operating state of the inverter supervisor. */
enum class OperatingState : std::uint8_t
{
    Standby,              ///< De-energized and waiting for ignition.
    Precharging,          ///< Charging the DC link through the precharge resistor.
    MainContactorClosing, ///< Overlapping relays during mechanical contactor pull-in.
    Ready,                ///< DC link energized; drive request not active.
    Driving,              ///< PWM enabled and torque command accepted.
    FaultLatched          ///< Safe outputs applied until guarded reset succeeds.
};

/** @brief Protection reason retained while the controller is fault-latched. */
enum class FaultCode : std::uint8_t
{
    None,                    ///< No fault is latched.
    EmergencyStop,           ///< Hardware/operator emergency stop is asserted.
    BatteryUndervoltage,     ///< Battery is too low to start safely.
    PrechargeTimeout,        ///< DC link did not charge within the allowed time.
    DcLinkUndervoltage,      ///< Energized DC link fell below its safe ratio.
    InverterOvertemperature, ///< Power-stage temperature exceeded its limit.
    PhaseOvercurrent         ///< Absolute measured phase current exceeded its limit.
};

/** @brief Values sampled by the hardware abstraction before a periodic tick. */
struct Inputs
{
    bool ignitionOn{false}; ///< Ignition/key state.
    bool driveRequest{false}; ///< Request transition between Ready and Driving.
    bool faultResetRequest{false}; ///< One-tick request to clear a safe latched fault.
    bool emergencyStopActive{false}; ///< Highest-priority stop input.
    float batteryVoltageV{0.0F}; ///< Battery voltage in volts.
    float dcLinkVoltageV{0.0F}; ///< DC-link voltage in volts.
    float inverterTemperatureC{25.0F}; ///< Power-stage temperature in degrees Celsius.
    float phaseCurrentA{0.0F}; ///< Phase current in amperes.
    float torqueRequestNm{0.0F}; ///< Requested torque in newton-metres.
};

/** @brief Commands written to the hardware abstraction after a periodic tick. */
struct Outputs
{
    bool prechargeRelayClosed{false}; ///< Energize the precharge relay coil.
    bool mainContactorClosed{false}; ///< Energize the main contactor coil.
    bool pwmEnabled{false}; ///< Enable power-stage gate pulses.
    bool faultLampOn{false}; ///< Illuminate the fault indicator.
    float torqueCommandNm{0.0F}; ///< Limited torque command in newton-metres.
};

/**
 * @brief Inverter supervisory State-pattern context.
 *
 * State objects contain behavior only and are immutable static instances. All
 * mutable data lives in this context, so construction and periodic tick() calls
 * perform no heap allocation.
 */
class InverterController final
{
public:
    /** @brief Construct the controller in Standby with safe outputs. */
    InverterController() noexcept;
    /**
     * @brief Run one supervisory update.
     * @param inputs Complete sample for this cycle.
     * @param elapsedMs Monotonic elapsed duration since the previous tick.
     */
    void tick(const Inputs& inputs, std::uint32_t elapsedMs) noexcept;

    /** @return Current externally visible state. */
    [[nodiscard]] OperatingState state() const noexcept;
    /** @return Latched fault, or FaultCode::None. */
    [[nodiscard]] FaultCode fault() const noexcept;
    /** @return Current output commands, valid for this object's lifetime. */
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

/** @return Stable human-readable name for @p state. */
[[nodiscard]] const char* toString(OperatingState state) noexcept;
/** @return Stable human-readable name for @p fault. */
[[nodiscard]] const char* toString(FaultCode fault) noexcept;
/** @} */
} // namespace inverter
