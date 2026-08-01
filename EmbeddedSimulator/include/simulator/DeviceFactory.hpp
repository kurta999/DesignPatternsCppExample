#pragma once

#include "simulator/SimulationTypes.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace simulator
{
class IMeasurementDriver
{
public:
    virtual ~IMeasurementDriver() = default;
    [[nodiscard]] virtual SensorReadings sample(const PlantState& plant,
                                                std::uint64_t timeMs) const noexcept = 0;
    [[nodiscard]] virtual std::string description() const = 0;
};

class ICoolingStrategy
{
public:
    virtual ~ICoolingStrategy() = default;
    [[nodiscard]] virtual float fanDutyPercent(float temperatureC) const noexcept = 0;
    [[nodiscard]] virtual const char* name() const noexcept = 0;
};

class DeviceFactory final
{
public:
    [[nodiscard]] static std::unique_ptr<IMeasurementDriver> createMeasurementDriver(
        const SimulationConfiguration& configuration);
    [[nodiscard]] static std::unique_ptr<ICoolingStrategy> createCoolingStrategy(
        CoolingMode mode);
};
} // namespace simulator

