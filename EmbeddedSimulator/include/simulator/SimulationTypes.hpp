#pragma once

/**
 * @file SimulationTypes.hpp
 * @brief Shared configuration, operator input, plant, measurement, and snapshot types.
 */

#include "firmware/InverterController.hpp"

#include <cstdint>

namespace simulator
{
/** @addtogroup connected_simulator
 *  @{
 */
/** @brief Nominal electrical plant to simulate. */
enum class DeviceProfile : std::uint8_t
{
    Traction400V, ///< 400 V passenger-vehicle traction inverter.
    Traction800V  ///< 800 V high-voltage traction inverter.
};

/** @brief Supported embedded control-board families. */
enum class BoardFamily : std::uint8_t
{
    Stm32H7, ///< STM32H7 peripherals and conversion characteristics.
    NxpS32K3 ///< NXP S32K3 peripherals and conversion characteristics.
};

/** @brief Voltage/current acquisition frontend. */
enum class AnalogFrontend : std::uint8_t
{
    OnChipAdc, ///< MCU-integrated successive-approximation ADC.
    IsolatedAdc ///< External isolated sigma-delta converter.
};

/** @brief Temperature sensing technology connected to the board. */
enum class TemperatureSensor : std::uint8_t
{
    Ntc10K, ///< 10 kOhm negative-temperature-coefficient thermistor.
    Pt100   ///< Platinum 100 Ohm resistance-temperature detector.
};

/** @brief Runtime-selectable cooling policy. */
enum class CoolingMode : std::uint8_t
{
    Quiet,      ///< Limit fan speed until greater cooling is necessary.
    Performance ///< Start cooling earlier and permit full fan duty.
};

/** @brief Complete hardware and policy selection for one simulation run. */
struct SimulationConfiguration
{
    DeviceProfile profile{DeviceProfile::Traction400V}; ///< Electrical plant profile.
    BoardFamily board{BoardFamily::Stm32H7}; ///< Board-support family.
    AnalogFrontend analogFrontend{AnalogFrontend::OnChipAdc}; ///< Acquisition frontend.
    TemperatureSensor temperatureSensor{TemperatureSensor::Ntc10K}; ///< Temperature device.
    CoolingMode coolingMode{CoolingMode::Quiet}; ///< Fan-control Strategy selection.
};

/** @brief Operator requests and deterministic fault-injection controls. */
struct SimulationControls
{
    bool ignitionOn{false}; ///< Simulated key/ignition input.
    bool driveRequest{false}; ///< Request PWM-enabled Driving state.
    bool emergencyStopActive{false}; ///< Assert the emergency-stop input.
    bool forcePrechargeOpenCircuit{false}; ///< Prevent the DC link from charging through precharge.
    bool forceOvertemperature{false}; ///< Force the plant above its thermal trip threshold.
    bool forcePhaseOvercurrent{false}; ///< Force the plant above its current trip threshold.
    float requestedTorqueNm{0.0F}; ///< Requested shaft torque in newton-metres.
};

/** @brief Internal ground-truth state of the deterministic plant model. */
struct PlantState
{
    float batteryVoltageV{400.0F}; ///< Battery terminal voltage in volts.
    float dcLinkVoltageV{0.0F}; ///< Inverter DC-link voltage in volts.
    float inverterTemperatureC{25.0F}; ///< Power-stage temperature in degrees Celsius.
    float phaseCurrentA{0.0F}; ///< Absolute simulated phase current in amperes.
};

/**
 * @brief Physical-unit values produced by an IMeasurementDriver.
 *
 * These values include the selected frontend's deterministic quantization,
 * noise, and sensor bias and are the inputs seen by the controller.
 */
struct SensorReadings
{
    float batteryVoltageV{0.0F}; ///< Measured battery voltage in volts.
    float dcLinkVoltageV{0.0F}; ///< Measured DC-link voltage in volts.
    float inverterTemperatureC{0.0F}; ///< Measured temperature in degrees Celsius.
    float phaseCurrentA{0.0F}; ///< Measured absolute phase current in amperes.
};

/** @brief Immutable view published to observers after a simulation step. */
struct SimulationSnapshot
{
    std::uint64_t simulationTimeMs{0U}; ///< Monotonic simulated time in milliseconds.
    SensorReadings measurements{}; ///< Values presented to the inverter controller.
    float fanDutyPercent{0.0F}; ///< Cooling Strategy result in the range 0..100 percent.
    float torqueCommandNm{0.0F}; ///< Safety-limited controller output in newton-metres.
    inverter::OperatingState operatingState{inverter::OperatingState::Standby}; ///< State result.
    inverter::FaultCode fault{inverter::FaultCode::None}; ///< Currently latched fault.
    bool prechargeRelayClosed{false}; ///< Precharge relay output.
    bool mainContactorClosed{false}; ///< Main contactor output.
    bool pwmEnabled{false}; ///< PWM gate-enable output.
};

/** @name Human-readable enum conversion */
///@{
[[nodiscard]] const char* toString(DeviceProfile value) noexcept;
[[nodiscard]] const char* toString(BoardFamily value) noexcept;
[[nodiscard]] const char* toString(AnalogFrontend value) noexcept;
[[nodiscard]] const char* toString(TemperatureSensor value) noexcept;
[[nodiscard]] const char* toString(CoolingMode value) noexcept;
///@}
/** @} */
} // namespace simulator
