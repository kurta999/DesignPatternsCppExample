#pragma once

/**
 * @file SimulationEngine.hpp
 * @brief wx-independent Facade and Mediator for the connected simulator.
 */

#include "simulator/Logging.hpp"
#include "simulator/SimulationTypes.hpp"

#include <cstdint>
#include <memory>

namespace simulator
{
/** @addtogroup connected_simulator
 *  @{
 */
/** @brief Observer notified synchronously after configuration, reset, or a step. */
class ISimulationObserver
{
public:
    virtual ~ISimulationObserver() = default;
    /**
     * @brief Receive the latest immutable simulator view.
     * @param snapshot View valid for the duration of the callback.
     * @note Observers are called on the thread that called SimulationEngine.
     */
    virtual void onSimulationSnapshot(const SimulationSnapshot& snapshot) = 0;
};

/** @brief Receiver interface used by queued simulation Commands. */
class ISimulationControl
{
public:
    virtual ~ISimulationControl() = default;
    /** @brief Allow subsequent step() calls to advance simulated time. */
    virtual void start() = 0;
    /** @brief Stop time advancement without modifying plant or controller state. */
    virtual void pause() = 0;
    /** @brief Return plant, controls, time, and controller to initial state. */
    virtual void reset() = 0;
    /** @brief Pulse a fault-reset request during the next simulation step. */
    virtual void requestFaultReset() = 0;
};

/**
 * @brief Application Facade and Mediator for all simulation collaborators.
 *
 * The GUI sees one API while this class coordinates the electrical/thermal
 * plant, measurement Adapter, cooling Strategy, inverter State context,
 * observers, and logger. Its implementation contains no wxWidgets types.
 *
 * @par Ownership
 * The engine owns its selected driver and Strategy. It does not own the injected
 * logger or attached observers; those objects must outlive the engine.
 *
 * @par Threading
 * The engine is intentionally single-threaded. Call all methods and observer
 * callbacks from the GUI/event-loop thread or provide external synchronization.
 */
class SimulationEngine final : public ISimulationControl
{
public:
    /** @param logger Non-owning logger reference that must outlive this object. */
    explicit SimulationEngine(ILogger& logger);
    ~SimulationEngine();
    SimulationEngine(const SimulationEngine&) = delete;
    SimulationEngine& operator=(const SimulationEngine&) = delete;

    /**
     * @brief Rebuild selected devices, reset the model, log, and publish a snapshot.
     * @param configuration New hardware and policy selection.
     */
    void applyConfiguration(const SimulationConfiguration& configuration);
    /** @brief Replace operator and fault-injection controls for future steps. */
    void setControls(const SimulationControls& controls) noexcept;
    /**
     * @brief Attach a non-owning Observer, ignoring duplicate registration.
     * @param observer Observer that must outlive this engine.
     */
    void attachObserver(ISimulationObserver& observer);

    /** @copydoc ISimulationControl::start */
    void start() override;
    /** @copydoc ISimulationControl::pause */
    void pause() override;
    /** @copydoc ISimulationControl::reset */
    void reset() override;
    /** @copydoc ISimulationControl::requestFaultReset */
    void requestFaultReset() override;
    /**
     * @brief Advance the connected model when running.
     * @param elapsedMs Requested elapsed time, clamped to 1..1000 ms.
     * @note Does nothing while paused.
     */
    void step(std::uint32_t elapsedMs);

    /** @return Whether step() currently advances simulated time. */
    [[nodiscard]] bool isRunning() const noexcept;
    /** @return Active configuration; valid for this object's lifetime. */
    [[nodiscard]] const SimulationConfiguration& configuration() const noexcept;
    /** @return Most recently published snapshot; valid for this object's lifetime. */
    [[nodiscard]] const SimulationSnapshot& snapshot() const noexcept;
    /** @return Human-readable description of the selected measurement Adapter. */
    [[nodiscard]] std::string driverDescription() const;
    /** @return Human-readable name of the selected cooling Strategy. */
    [[nodiscard]] std::string coolingStrategyName() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
/** @} */
} // namespace simulator
