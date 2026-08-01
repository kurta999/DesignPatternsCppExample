#include "catalog/PatternDemo.hpp"
#include "firmware/InverterController.hpp"

namespace catalog::behavioral
{
DemoResult runState()
{
    inverter::InverterController controller;
    inverter::Inputs inputs;
    inputs.batteryVoltageV = 400.0F;
    inputs.inverterTemperatureC = 42.0F;
    inputs.ignitionOn = true;
    controller.tick(inputs, 10U);
    require(controller.state() == inverter::OperatingState::Precharging, "precharge did not start");
    inputs.dcLinkVoltageV = 365.0F;
    controller.tick(inputs, 100U);
    require(controller.state() == inverter::OperatingState::MainContactorClosing,
            "contactor overlap did not start");
    controller.tick(inputs, 100U);
    require(controller.state() == inverter::OperatingState::Ready, "Ready was not reached");
    inputs.driveRequest = true;
    controller.tick(inputs, 10U);
    require(controller.state() == inverter::OperatingState::Driving && controller.outputs().pwmEnabled,
            "Driving did not enable PWM");
    return {"Behavioral", "State",
            "The inverter progressed through precharge, contactor overlap, Ready, and Driving."};
}
}

