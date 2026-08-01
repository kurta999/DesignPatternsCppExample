#include "simulator/Logging.hpp"
#include "simulator/SimulationCommands.hpp"
#include "simulator/SimulationEngine.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace
{
void require(const bool condition, const std::string_view message)
{
    if (!condition) throw std::logic_error(std::string{message});
}

class SnapshotCounter final : public simulator::ISimulationObserver
{
public:
    void onSimulationSnapshot(const simulator::SimulationSnapshot&) override { ++count_; }
    [[nodiscard]] std::size_t count() const noexcept { return count_; }
private:
    std::size_t count_{0U};
};

void runEveryHardwareCombination(simulator::SimulationEngine& engine)
{
    using namespace simulator;
    for (const auto profile : {DeviceProfile::Traction400V, DeviceProfile::Traction800V})
    for (const auto board : {BoardFamily::Stm32H7, BoardFamily::NxpS32K3})
    for (const auto frontend : {AnalogFrontend::OnChipAdc, AnalogFrontend::IsolatedAdc})
    for (const auto sensor : {TemperatureSensor::Ntc10K, TemperatureSensor::Pt100})
    for (const auto cooling : {CoolingMode::Quiet, CoolingMode::Performance})
    {
        engine.applyConfiguration({profile, board, frontend, sensor, cooling});
        require(!engine.driverDescription().empty(), "factory returned an unnamed driver");
        require(!engine.coolingStrategyName().empty(), "factory returned an unnamed strategy");

        SimulationControls controls;
        controls.ignitionOn = true;
        engine.setControls(controls);
        engine.start();
        for (int cycle = 0; cycle < 40; ++cycle) engine.step(100U);
        require(engine.snapshot().operatingState == inverter::OperatingState::Ready,
                "configured hardware did not reach Ready");

        controls.driveRequest = true;
        controls.requestedTorqueNm = 140.0F;
        engine.setControls(controls);
        for (int cycle = 0; cycle < 3; ++cycle) engine.step(100U);
        require(engine.snapshot().operatingState == inverter::OperatingState::Driving &&
                    engine.snapshot().pwmEnabled,
                "drive request did not enable the inverter");

        controls.forcePhaseOvercurrent = true;
        engine.setControls(controls);
        engine.step(100U);
        require(engine.snapshot().fault == inverter::FaultCode::PhaseOvercurrent,
                "overcurrent injection did not trip protection");

        controls.ignitionOn = false;
        controls.driveRequest = false;
        controls.forcePhaseOvercurrent = false;
        engine.setControls(controls);
        // The fault-reset contract requires measured phase current below 5 A.
        // Let the simulated current decay instead of bypassing that safety check.
        for (int cycle = 0; cycle < 3; ++cycle) engine.step(100U);
        require(engine.snapshot().measurements.phaseCurrentA < 5.0F,
                "phase current did not decay to a safe reset level");
        SimulationCommandQueue commands;
        commands.enqueue(std::make_unique<FaultResetCommand>());
        commands.dispatchAll(engine);
        engine.step(100U);
        require(engine.snapshot().fault == inverter::FaultCode::None &&
                    engine.snapshot().operatingState == inverter::OperatingState::Standby,
                "safe fault reset did not return to Standby");
    }
}

void runPrechargeFailure(simulator::SimulationEngine& engine)
{
    simulator::SimulationControls controls;
    controls.ignitionOn = true;
    controls.forcePrechargeOpenCircuit = true;
    engine.applyConfiguration({});
    engine.setControls(controls);
    engine.start();
    for (int cycle = 0; cycle < 20; ++cycle) engine.step(100U);
    require(engine.snapshot().fault == inverter::FaultCode::PrechargeTimeout,
            "open precharge circuit did not time out");
}
} // namespace

int main()
{
    try
    {
        simulator::Logger logger;
        auto memoryLog = std::make_shared<simulator::MemoryLogSink>();
        logger.addSink(memoryLog);
        simulator::SimulationEngine engine{logger};
        SnapshotCounter snapshots;
        engine.attachObserver(snapshots);

        runEveryHardwareCombination(engine);
        runPrechargeFailure(engine);
        require(snapshots.count() > 1'000U, "observer did not receive periodic snapshots");
        require(memoryLog->records().size() > 100U, "logging pipeline did not capture events");
        std::cout << "All connected simulator engine scenarios passed.\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Simulator test failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
