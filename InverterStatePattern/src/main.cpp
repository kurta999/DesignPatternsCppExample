/**
 * @file
 * @brief Standalone scenarios for inverter startup, faults, and guarded reset.
 */

#include "firmware/InverterController.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
using inverter::FaultCode;
using inverter::Inputs;
using inverter::InverterController;
using inverter::OperatingState;

void require(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "CHECK FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

Inputs nominalInputs()
{
    Inputs inputs;
    inputs.batteryVoltageV = 400.0F;
    inputs.dcLinkVoltageV = 365.0F;
    inputs.inverterTemperatureC = 45.0F;
    return inputs;
}

void bringToReady(InverterController& controller, Inputs& inputs)
{
    inputs.ignitionOn = true;
    inputs.dcLinkVoltageV = 0.0F;
    controller.tick(inputs, 10U);
    require(controller.state() == OperatingState::Precharging,
            "ignition should start precharge");

    inputs.dcLinkVoltageV = 365.0F;
    controller.tick(inputs, 100U);
    require(controller.state() == OperatingState::MainContactorClosing,
            "90% DC-link voltage should close the main contactor");
    require(controller.outputs().prechargeRelayClosed &&
                controller.outputs().mainContactorClosed,
            "precharge must overlap main contactor pull-in");

    controller.tick(inputs, 100U);
    require(controller.state() == OperatingState::Ready,
            "main contactor should settle before Ready");
}

void runNormalDriveScenario()
{
    std::cout << "Scenario 1 - normal startup and drive\n";
    InverterController controller;
    Inputs inputs = nominalInputs();
    bringToReady(controller, inputs);
    inputs.driveRequest = true;
    inputs.torqueRequestNm = 300.0F;
    controller.tick(inputs, 10U);
    controller.tick(inputs, 10U);
    require(controller.state() == OperatingState::Driving &&
                controller.outputs().pwmEnabled,
            "drive request should enable PWM");
    require(std::fabs(controller.outputs().torqueCommandNm - 220.0F) < 0.01F,
            "torque request should be clamped");
}

void runPrechargeFailureScenario()
{
    std::cout << "Scenario 2 - DC link fails to precharge\n";
    InverterController controller;
    Inputs inputs = nominalInputs();
    inputs.ignitionOn = true;
    inputs.dcLinkVoltageV = 70.0F;
    controller.tick(inputs, 10U);
    for (int cycle = 0; cycle < 15; ++cycle)
    {
        controller.tick(inputs, 100U);
    }
    require(controller.state() == OperatingState::FaultLatched &&
                controller.fault() == FaultCode::PrechargeTimeout,
            "stalled precharge must latch a timeout fault");
    require(controller.outputs().faultLampOn &&
                !controller.outputs().mainContactorClosed,
            "fault must immediately command safe outputs");
}

void runThermalResetScenario()
{
    std::cout << "Scenario 3 - thermal trip and guarded reset\n";
    InverterController controller;
    Inputs inputs = nominalInputs();
    bringToReady(controller, inputs);
    inputs.driveRequest = true;
    controller.tick(inputs, 10U);
    inputs.inverterTemperatureC = 110.0F;
    controller.tick(inputs, 10U);
    require(controller.fault() == FaultCode::InverterOvertemperature,
            "overtemperature must latch a fault");

    inputs.faultResetRequest = true;
    inputs.inverterTemperatureC = 70.0F;
    controller.tick(inputs, 10U);
    require(controller.state() == OperatingState::FaultLatched,
            "reset must be rejected while ignition is on");
    inputs.ignitionOn = false;
    inputs.driveRequest = false;
    controller.tick(inputs, 10U);
    require(controller.state() == OperatingState::Standby &&
                controller.fault() == FaultCode::None,
            "safe reset should return to Standby");
}
} // namespace

int main()
{
    runNormalDriveScenario();
    runPrechargeFailureScenario();
    runThermalResetScenario();
    std::cout << "All embedded State pattern scenarios passed.\n";
    return EXIT_SUCCESS;
}
