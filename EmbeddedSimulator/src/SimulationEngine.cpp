#include "simulator/SimulationEngine.hpp"

#include "simulator/DeviceFactory.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace simulator
{
class SimulationEngine::Impl
{
public:
    explicit Impl(ILogger& logger) : logger_{logger}
    {
        rebuildDevices();
        resetModel(false);
    }

    void applyConfiguration(const SimulationConfiguration& configuration)
    {
        configuration_ = configuration;
        rebuildDevices();
        resetModel(false);
        std::ostringstream text;
        text << "Configured " << toString(configuration_.profile) << ", "
             << toString(configuration_.board) << ", "
             << toString(configuration_.analogFrontend) << ", "
             << toString(configuration_.temperatureSensor) << ", "
             << cooling_->name() << ". Driver: " << measurementDriver_->description();
        logger_.log(LogLevel::Info, "Configuration", text.str());
        notifyObservers();
    }

    void start()
    {
        if (running_) return;
        running_ = true;
        logger_.log(LogLevel::Info, "Simulation", "Simulation started");
    }
    void pause()
    {
        if (!running_) return;
        running_ = false;
        logger_.log(LogLevel::Info, "Simulation", "Simulation paused");
    }
    void reset()
    {
        resetModel(true);
        notifyObservers();
    }

    void step(const std::uint32_t requestedElapsedMs)
    {
        if (!running_) return;
        const std::uint32_t elapsedMs = std::clamp(requestedElapsedMs, 1U, 1'000U);
        updatePlant(elapsedMs);
        simulationTimeMs_ += elapsedMs;

        const auto readings = measurementDriver_->sample(plant_, simulationTimeMs_);
        const float fanDuty = cooling_->fanDutyPercent(readings.inverterTemperatureC);
        inverter::Inputs inverterInputs;
        inverterInputs.ignitionOn = controls_.ignitionOn;
        inverterInputs.driveRequest = controls_.driveRequest;
        inverterInputs.faultResetRequest = faultResetRequested_;
        inverterInputs.emergencyStopActive = controls_.emergencyStopActive;
        inverterInputs.batteryVoltageV = readings.batteryVoltageV;
        inverterInputs.dcLinkVoltageV = readings.dcLinkVoltageV;
        inverterInputs.inverterTemperatureC = readings.inverterTemperatureC;
        inverterInputs.phaseCurrentA = readings.phaseCurrentA;
        inverterInputs.torqueRequestNm = controls_.requestedTorqueNm;
        controller_.tick(inverterInputs, elapsedMs);
        faultResetRequested_ = false;

        const auto& outputs = controller_.outputs();
        snapshot_ = {simulationTimeMs_, readings, fanDuty, outputs.torqueCommandNm,
                     controller_.state(), controller_.fault(),
                     outputs.prechargeRelayClosed, outputs.mainContactorClosed,
                     outputs.pwmEnabled};
        logStateChanges();
        if (simulationTimeMs_ >= nextTelemetryLogMs_)
        {
            logTelemetry();
            nextTelemetryLogMs_ = simulationTimeMs_ + 1'000U;
        }
        notifyObservers();
    }

    void resetModel(const bool writeLog)
    {
        running_ = false;
        controls_ = {};
        faultResetRequested_ = false;
        simulationTimeMs_ = 0U;
        nextTelemetryLogMs_ = 1'000U;
        controller_ = inverter::InverterController{};
        plant_ = {};
        plant_.batteryVoltageV = nominalBatteryVoltage();
        const auto readings = measurementDriver_->sample(plant_, 0U);
        snapshot_ = {0U, readings, cooling_->fanDutyPercent(readings.inverterTemperatureC),
                     0.0F, controller_.state(), controller_.fault(), false, false, false};
        lastState_ = controller_.state();
        lastFault_ = controller_.fault();
        if (writeLog) logger_.log(LogLevel::Info, "Simulation", "Plant and controller reset");
    }

    void updatePlant(const std::uint32_t elapsedMs)
    {
        const float elapsedSeconds = static_cast<float>(elapsedMs) / 1'000.0F;
        const auto& outputs = controller_.outputs();

        if (outputs.mainContactorClosed)
        {
            const float alpha = std::min(static_cast<float>(elapsedMs) / 80.0F, 1.0F);
            plant_.dcLinkVoltageV += (nominalBatteryVoltage() - plant_.dcLinkVoltageV) * alpha;
        }
        else if (outputs.prechargeRelayClosed && !controls_.forcePrechargeOpenCircuit)
        {
            const float alpha = std::min(static_cast<float>(elapsedMs) / 500.0F, 1.0F);
            plant_.dcLinkVoltageV += (nominalBatteryVoltage() - plant_.dcLinkVoltageV) * alpha;
        }
        else
        {
            const float discharge = std::min(static_cast<float>(elapsedMs) / 1'500.0F, 1.0F);
            plant_.dcLinkVoltageV *= (1.0F - discharge);
        }

        if (controls_.forcePhaseOvercurrent)
            plant_.phaseCurrentA = 520.0F;
        else if (outputs.pwmEnabled)
            plant_.phaseCurrentA = std::fabs(controls_.requestedTorqueNm) * 1.4F;
        else
            plant_.phaseCurrentA *= std::max(0.0F, 1.0F - (elapsedSeconds * 8.0F));

        plant_.batteryVoltageV = nominalBatteryVoltage() -
                                 (std::fabs(plant_.phaseCurrentA) * 0.015F);

        if (controls_.forceOvertemperature)
        {
            plant_.inverterTemperatureC = 112.0F;
        }
        else
        {
            const float heatingCPerSecond = outputs.pwmEnabled
                ? (std::fabs(outputs.torqueCommandNm) / 220.0F) * 1.8F : 0.0F;
            const float fanCoolingCPerSecond = (snapshot_.fanDutyPercent / 100.0F) * 5.0F;
            const float passiveCoolingCPerSecond =
                std::max(0.0F, plant_.inverterTemperatureC - 25.0F) * 0.025F;
            plant_.inverterTemperatureC +=
                (heatingCPerSecond - fanCoolingCPerSecond - passiveCoolingCPerSecond) *
                elapsedSeconds;
            plant_.inverterTemperatureC =
                std::clamp(plant_.inverterTemperatureC, 25.0F, 140.0F);
        }
    }

    void logStateChanges()
    {
        if (snapshot_.operatingState != lastState_)
        {
            std::ostringstream text;
            text << inverter::toString(lastState_) << " -> "
                 << inverter::toString(snapshot_.operatingState);
            logger_.log(LogLevel::Info, "Inverter State", text.str());
            lastState_ = snapshot_.operatingState;
        }
        if (snapshot_.fault != lastFault_)
        {
            const auto level = snapshot_.fault == inverter::FaultCode::None
                                   ? LogLevel::Info : LogLevel::Error;
            logger_.log(level, "Protection", inverter::toString(snapshot_.fault));
            lastFault_ = snapshot_.fault;
        }
    }

    void logTelemetry()
    {
        std::ostringstream text;
        text.setf(std::ios::fixed);
        text.precision(1);
        text << "Battery=" << snapshot_.measurements.batteryVoltageV
             << " V, DC-link=" << snapshot_.measurements.dcLinkVoltageV
             << " V, Temp=" << snapshot_.measurements.inverterTemperatureC
             << " C, Current=" << snapshot_.measurements.phaseCurrentA
             << " A, Fan=" << snapshot_.fanDutyPercent << "%";
        logger_.log(LogLevel::Debug, "Measurements", text.str());
    }

    void notifyObservers()
    {
        for (auto* observer : observers_) observer->onSimulationSnapshot(snapshot_);
    }

    void rebuildDevices()
    {
        measurementDriver_ = DeviceFactory::createMeasurementDriver(configuration_);
        cooling_ = DeviceFactory::createCoolingStrategy(configuration_.coolingMode);
    }
    float nominalBatteryVoltage() const noexcept
    {
        return configuration_.profile == DeviceProfile::Traction800V ? 800.0F : 400.0F;
    }

    ILogger& logger_;
    SimulationConfiguration configuration_{};
    SimulationControls controls_{};
    PlantState plant_{};
    inverter::InverterController controller_{};
    std::unique_ptr<IMeasurementDriver> measurementDriver_;
    std::unique_ptr<ICoolingStrategy> cooling_;
    std::vector<ISimulationObserver*> observers_;
    SimulationSnapshot snapshot_{};
    inverter::OperatingState lastState_{inverter::OperatingState::Standby};
    inverter::FaultCode lastFault_{inverter::FaultCode::None};
    std::uint64_t simulationTimeMs_{0U};
    std::uint64_t nextTelemetryLogMs_{1'000U};
    bool running_{false};
    bool faultResetRequested_{false};
};

SimulationEngine::SimulationEngine(ILogger& logger) : impl_{std::make_unique<Impl>(logger)} {}
SimulationEngine::~SimulationEngine() = default;
void SimulationEngine::applyConfiguration(const SimulationConfiguration& configuration)
{ impl_->applyConfiguration(configuration); }
void SimulationEngine::setControls(const SimulationControls& controls) noexcept
{ impl_->controls_ = controls; }
void SimulationEngine::attachObserver(ISimulationObserver& observer)
{
    if (std::find(impl_->observers_.begin(), impl_->observers_.end(), &observer) ==
        impl_->observers_.end()) impl_->observers_.push_back(&observer);
}
void SimulationEngine::start() { impl_->start(); }
void SimulationEngine::pause() { impl_->pause(); }
void SimulationEngine::reset() { impl_->reset(); }
void SimulationEngine::requestFaultReset() { impl_->faultResetRequested_ = true; }
void SimulationEngine::step(const std::uint32_t elapsedMs) { impl_->step(elapsedMs); }
bool SimulationEngine::isRunning() const noexcept { return impl_->running_; }
const SimulationConfiguration& SimulationEngine::configuration() const noexcept
{ return impl_->configuration_; }
const SimulationSnapshot& SimulationEngine::snapshot() const noexcept { return impl_->snapshot_; }
std::string SimulationEngine::driverDescription() const { return impl_->measurementDriver_->description(); }
std::string SimulationEngine::coolingStrategyName() const { return impl_->cooling_->name(); }
} // namespace simulator

