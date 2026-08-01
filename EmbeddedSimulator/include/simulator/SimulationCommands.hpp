#pragma once

/**
 * @file SimulationCommands.hpp
 * @brief Command objects and FIFO used to decouple GUI actions from the engine.
 */

#include "simulator/SimulationEngine.hpp"

#include <memory>
#include <queue>

namespace simulator
{
/** @addtogroup connected_simulator
 *  @{
 */
/** @brief Polymorphic operator request executed against ISimulationControl. */
class ISimulationCommand
{
public:
    virtual ~ISimulationCommand() = default;
    /** @param target Non-owning receiver for this command. */
    virtual void execute(ISimulationControl& target) = 0;
};

/** @brief Command that starts time advancement. */
class StartCommand final : public ISimulationCommand
{
public:
    /** @copydoc ISimulationCommand::execute */
    void execute(ISimulationControl& target) override { target.start(); }
};
/** @brief Command that pauses time advancement. */
class PauseCommand final : public ISimulationCommand
{
public:
    /** @copydoc ISimulationCommand::execute */
    void execute(ISimulationControl& target) override { target.pause(); }
};
/** @brief Command that resets the plant and controller. */
class ResetCommand final : public ISimulationCommand
{
public:
    /** @copydoc ISimulationCommand::execute */
    void execute(ISimulationControl& target) override { target.reset(); }
};
/** @brief Command that requests a guarded fault reset on the next step. */
class FaultResetCommand final : public ISimulationCommand
{
public:
    /** @copydoc ISimulationCommand::execute */
    void execute(ISimulationControl& target) override { target.requestFaultReset(); }
};

/** @brief Owning FIFO Invoker for pending simulation Commands. */
class SimulationCommandQueue final
{
public:
    /**
     * @brief Transfer ownership of a Command into the queue.
     * @param command Command to execute during a future dispatch.
     * @pre @p command must not be null.
     */
    void enqueue(std::unique_ptr<ISimulationCommand> command)
    {
        commands_.push(std::move(command));
    }
    /**
     * @brief Execute queued Commands in FIFO order and empty the queue.
     * @param target Receiver used for every pending Command.
     */
    void dispatchAll(ISimulationControl& target)
    {
        while (!commands_.empty())
        {
            commands_.front()->execute(target);
            commands_.pop();
        }
    }
private:
    std::queue<std::unique_ptr<ISimulationCommand>> commands_;
};
/** @} */
} // namespace simulator
