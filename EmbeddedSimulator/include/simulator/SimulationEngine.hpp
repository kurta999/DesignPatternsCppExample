#pragma once

#include "simulator/Logging.hpp"
#include "simulator/SimulationTypes.hpp"

#include <cstdint>
#include <memory>

namespace simulator
{
class ISimulationObserver
{
public:
    virtual ~ISimulationObserver() = default;
    virtual void onSimulationSnapshot(const SimulationSnapshot& snapshot) = 0;
};

class ISimulationControl
{
public:
    virtual ~ISimulationControl() = default;
    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void reset() = 0;
    virtual void requestFaultReset() = 0;
};

// Facade + Mediator: the GUI sees one API while the engine coordinates the plant,
// measurement drivers, cooling strategy, inverter State context, observers, and log.
class SimulationEngine final : public ISimulationControl
{
public:
    explicit SimulationEngine(ILogger& logger);
    ~SimulationEngine();
    SimulationEngine(const SimulationEngine&) = delete;
    SimulationEngine& operator=(const SimulationEngine&) = delete;

    void applyConfiguration(const SimulationConfiguration& configuration);
    void setControls(const SimulationControls& controls) noexcept;
    void attachObserver(ISimulationObserver& observer);

    void start() override;
    void pause() override;
    void reset() override;
    void requestFaultReset() override;
    void step(std::uint32_t elapsedMs);

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] const SimulationConfiguration& configuration() const noexcept;
    [[nodiscard]] const SimulationSnapshot& snapshot() const noexcept;
    [[nodiscard]] std::string driverDescription() const;
    [[nodiscard]] std::string coolingStrategyName() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace simulator

