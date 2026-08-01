#pragma once

#include "firmware/InverterController.hpp"

#include <cstdint>

namespace simulator
{
enum class DeviceProfile : std::uint8_t { Traction400V, Traction800V };
enum class BoardFamily : std::uint8_t { Stm32H7, NxpS32K3 };
enum class AnalogFrontend : std::uint8_t { OnChipAdc, IsolatedAdc };
enum class TemperatureSensor : std::uint8_t { Ntc10K, Pt100 };
enum class CoolingMode : std::uint8_t { Quiet, Performance };

struct SimulationConfiguration
{
    DeviceProfile profile{DeviceProfile::Traction400V};
    BoardFamily board{BoardFamily::Stm32H7};
    AnalogFrontend analogFrontend{AnalogFrontend::OnChipAdc};
    TemperatureSensor temperatureSensor{TemperatureSensor::Ntc10K};
    CoolingMode coolingMode{CoolingMode::Quiet};
};

struct SimulationControls
{
    bool ignitionOn{false};
    bool driveRequest{false};
    bool emergencyStopActive{false};
    bool forcePrechargeOpenCircuit{false};
    bool forceOvertemperature{false};
    bool forcePhaseOvercurrent{false};
    float requestedTorqueNm{0.0F};
};

struct PlantState
{
    float batteryVoltageV{400.0F};
    float dcLinkVoltageV{0.0F};
    float inverterTemperatureC{25.0F};
    float phaseCurrentA{0.0F};
};

struct SensorReadings
{
    float batteryVoltageV{0.0F};
    float dcLinkVoltageV{0.0F};
    float inverterTemperatureC{0.0F};
    float phaseCurrentA{0.0F};
};

struct SimulationSnapshot
{
    std::uint64_t simulationTimeMs{0U};
    SensorReadings measurements{};
    float fanDutyPercent{0.0F};
    float torqueCommandNm{0.0F};
    inverter::OperatingState operatingState{inverter::OperatingState::Standby};
    inverter::FaultCode fault{inverter::FaultCode::None};
    bool prechargeRelayClosed{false};
    bool mainContactorClosed{false};
    bool pwmEnabled{false};
};

[[nodiscard]] const char* toString(DeviceProfile value) noexcept;
[[nodiscard]] const char* toString(BoardFamily value) noexcept;
[[nodiscard]] const char* toString(AnalogFrontend value) noexcept;
[[nodiscard]] const char* toString(TemperatureSensor value) noexcept;
[[nodiscard]] const char* toString(CoolingMode value) noexcept;
} // namespace simulator

