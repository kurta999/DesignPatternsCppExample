#pragma once

/**
 * @file DeviceFactory.hpp
 * @brief Abstract measurement/cooling contracts and their runtime factory.
 */

#include "simulator/SimulationTypes.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace simulator
{
/** @addtogroup connected_simulator
 *  @{
 */
/**
 * @brief Adapter interface from a concrete acquisition frontend to physical units.
 *
 * Implementations hide board peripheral selection, resolution, quantization,
 * and sensor characteristics from SimulationEngine.
 */
class IMeasurementDriver
{
public:
    virtual ~IMeasurementDriver() = default;
    /**
     * @brief Sample the plant using this driver's acquisition characteristics.
     * @param plant Ground-truth plant values.
     * @param timeMs Monotonic simulation time, used for deterministic noise.
     * @return Controller-facing values expressed in physical units.
     */
    [[nodiscard]] virtual SensorReadings sample(const PlantState& plant,
                                                std::uint64_t timeMs) const noexcept = 0;
    /** @return Human-readable board, peripheral, sensor, and resolution description. */
    [[nodiscard]] virtual std::string description() const = 0;
};

/** @brief Strategy interface for interchangeable fan-control policies. */
class ICoolingStrategy
{
public:
    virtual ~ICoolingStrategy() = default;
    /**
     * @param temperatureC Measured inverter temperature in degrees Celsius.
     * @return Requested fan duty in the inclusive range 0..100 percent.
     */
    [[nodiscard]] virtual float fanDutyPercent(float temperatureC) const noexcept = 0;
    /** @return Stable, human-readable policy name. */
    [[nodiscard]] virtual const char* name() const noexcept = 0;
};

/**
 * @brief Factory entry point for configuration-dependent simulator devices.
 *
 * Measurement creation delegates to a board-specific Abstract Factory in the
 * implementation. Cooling creation selects a concrete Strategy.
 */
class DeviceFactory final
{
public:
    /**
     * @brief Build a board-compatible measurement Adapter.
     * @param configuration Selected board, frontend, sensor, and plant profile.
     * @return Exclusively owned measurement driver.
     * @throws std::invalid_argument if the board family is unsupported.
     */
    [[nodiscard]] static std::unique_ptr<IMeasurementDriver> createMeasurementDriver(
        const SimulationConfiguration& configuration);
    /**
     * @brief Build the selected cooling Strategy.
     * @param mode Cooling policy selection.
     * @return Exclusively owned cooling policy.
     * @throws std::invalid_argument if the mode is unsupported.
     */
    [[nodiscard]] static std::unique_ptr<ICoolingStrategy> createCoolingStrategy(
        CoolingMode mode);
};
/** @} */
} // namespace simulator
