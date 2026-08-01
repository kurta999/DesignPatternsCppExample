#pragma once

#include "simulator/SimulationEngine.hpp"

#include <memory>
#include <queue>

namespace simulator
{
class ISimulationCommand
{
public:
    virtual ~ISimulationCommand() = default;
    virtual void execute(ISimulationControl& target) = 0;
};

class StartCommand final : public ISimulationCommand
{
public:
    void execute(ISimulationControl& target) override { target.start(); }
};
class PauseCommand final : public ISimulationCommand
{
public:
    void execute(ISimulationControl& target) override { target.pause(); }
};
class ResetCommand final : public ISimulationCommand
{
public:
    void execute(ISimulationControl& target) override { target.reset(); }
};
class FaultResetCommand final : public ISimulationCommand
{
public:
    void execute(ISimulationControl& target) override { target.requestFaultReset(); }
};

class SimulationCommandQueue final
{
public:
    void enqueue(std::unique_ptr<ISimulationCommand> command)
    {
        commands_.push(std::move(command));
    }
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
} // namespace simulator

