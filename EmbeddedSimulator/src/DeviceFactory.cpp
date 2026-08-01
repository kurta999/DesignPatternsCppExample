#include "simulator/DeviceFactory.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace simulator
{
namespace
{
struct DriverParameters
{
    std::string board;
    std::string voltagePeripheral;
    std::string temperaturePeripheral;
    unsigned resolutionBits;
    float voltageNoiseV;
    float temperatureNoiseC;
    float currentNoiseA;
};

float quantizedSample(const float value, const float fullScale, const unsigned bits,
                      const float noiseAmplitude, const std::uint64_t timeMs,
                      const float phase) noexcept
{
    const float deterministicNoise = noiseAmplitude *
                                     std::sin((static_cast<float>(timeMs) * 0.013F) + phase);
    const float noisy = std::clamp(value + deterministicNoise, 0.0F, fullScale);
    const float maximumCounts = std::pow(2.0F, static_cast<float>(bits)) - 1.0F;
    const float counts = std::round((noisy / fullScale) * maximumCounts);
    return (counts / maximumCounts) * fullScale;
}

class SimulatedMeasurementDriver final : public IMeasurementDriver
{
public:
    SimulatedMeasurementDriver(DriverParameters parameters, const DeviceProfile profile,
                               const TemperatureSensor sensor)
        : parameters_{std::move(parameters)}, profile_{profile}, sensor_{sensor} {}

    SensorReadings sample(const PlantState& plant, const std::uint64_t timeMs) const noexcept override
    {
        const float voltageFullScale = profile_ == DeviceProfile::Traction800V ? 1'000.0F : 500.0F;
        const float sensorBias = sensor_ == TemperatureSensor::Ntc10K ? 0.35F : -0.08F;
        return {
            quantizedSample(plant.batteryVoltageV, voltageFullScale,
                            parameters_.resolutionBits, parameters_.voltageNoiseV, timeMs, 0.1F),
            quantizedSample(plant.dcLinkVoltageV, voltageFullScale,
                            parameters_.resolutionBits, parameters_.voltageNoiseV, timeMs, 1.7F),
            quantizedSample(plant.inverterTemperatureC + sensorBias, 160.0F,
                            parameters_.resolutionBits, parameters_.temperatureNoiseC, timeMs, 2.4F),
            quantizedSample(std::fabs(plant.phaseCurrentA), 600.0F,
                            parameters_.resolutionBits, parameters_.currentNoiseA, timeMs, 3.2F)};
    }

    std::string description() const override
    {
        std::ostringstream text;
        text << parameters_.board << " | " << parameters_.voltagePeripheral << " | "
             << parameters_.temperaturePeripheral << " | " << toString(sensor_)
             << " | " << parameters_.resolutionBits << "-bit";
        return text.str();
    }
private:
    DriverParameters parameters_;
    DeviceProfile profile_;
    TemperatureSensor sensor_;
};

// Abstract Factory: each board factory creates a driver family with compatible
// peripheral names and calibration characteristics.
class IBoardDriverFactory
{
public:
    virtual ~IBoardDriverFactory() = default;
    virtual std::unique_ptr<IMeasurementDriver> create(
        const SimulationConfiguration& configuration) const = 0;
};

class Stm32H7DriverFactory final : public IBoardDriverFactory
{
public:
    std::unique_ptr<IMeasurementDriver> create(
        const SimulationConfiguration& configuration) const override
    {
        const bool isolated = configuration.analogFrontend == AnalogFrontend::IsolatedAdc;
        DriverParameters parameters{"STM32H7 / FDCAN1",
            isolated ? "SPI2 isolated sigma-delta ADC" : "ADC3 internal frontend",
            configuration.temperatureSensor == TemperatureSensor::Ntc10K
                ? "ADC1 divider input" : "SPI3 RTD frontend",
            isolated ? 16U : 12U, isolated ? 0.025F : 0.22F,
            isolated ? 0.025F : 0.14F, isolated ? 0.08F : 0.65F};
        return std::make_unique<SimulatedMeasurementDriver>(
            std::move(parameters), configuration.profile, configuration.temperatureSensor);
    }
};

class NxpS32K3DriverFactory final : public IBoardDriverFactory
{
public:
    std::unique_ptr<IMeasurementDriver> create(
        const SimulationConfiguration& configuration) const override
    {
        const bool isolated = configuration.analogFrontend == AnalogFrontend::IsolatedAdc;
        DriverParameters parameters{"NXP S32K3 / FlexCAN0",
            isolated ? "LPSPI1 isolated sigma-delta ADC" : "SAR ADC0 internal frontend",
            configuration.temperatureSensor == TemperatureSensor::Ntc10K
                ? "ADC1 divider input" : "LPSPI2 RTD frontend",
            isolated ? 16U : 14U, isolated ? 0.020F : 0.12F,
            isolated ? 0.020F : 0.09F, isolated ? 0.07F : 0.35F};
        return std::make_unique<SimulatedMeasurementDriver>(
            std::move(parameters), configuration.profile, configuration.temperatureSensor);
    }
};

class QuietCoolingStrategy final : public ICoolingStrategy
{
public:
    float fanDutyPercent(const float temperatureC) const noexcept override
    { return std::clamp((temperatureC - 40.0F) * 2.0F, 0.0F, 60.0F); }
    const char* name() const noexcept override { return "Quiet strategy"; }
};
class PerformanceCoolingStrategy final : public ICoolingStrategy
{
public:
    float fanDutyPercent(const float temperatureC) const noexcept override
    { return std::clamp((temperatureC - 30.0F) * 3.0F, 20.0F, 100.0F); }
    const char* name() const noexcept override { return "Performance strategy"; }
};
} // namespace

std::unique_ptr<IMeasurementDriver> DeviceFactory::createMeasurementDriver(
    const SimulationConfiguration& configuration)
{
    std::unique_ptr<IBoardDriverFactory> factory;
    switch (configuration.board)
    {
    case BoardFamily::Stm32H7: factory = std::make_unique<Stm32H7DriverFactory>(); break;
    case BoardFamily::NxpS32K3: factory = std::make_unique<NxpS32K3DriverFactory>(); break;
    default: throw std::invalid_argument("unsupported board family");
    }
    return factory->create(configuration);
}

std::unique_ptr<ICoolingStrategy> DeviceFactory::createCoolingStrategy(const CoolingMode mode)
{
    switch (mode)
    {
    case CoolingMode::Quiet: return std::make_unique<QuietCoolingStrategy>();
    case CoolingMode::Performance: return std::make_unique<PerformanceCoolingStrategy>();
    }
    throw std::invalid_argument("unsupported cooling mode");
}
} // namespace simulator

