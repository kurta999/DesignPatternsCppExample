#include "simulator/SimulationTypes.hpp"

namespace simulator
{
const char* toString(const DeviceProfile value) noexcept
{
    switch (value)
    {
    case DeviceProfile::Traction400V: return "400 V traction inverter";
    case DeviceProfile::Traction800V: return "800 V traction inverter";
    }
    return "Unknown profile";
}
const char* toString(const BoardFamily value) noexcept
{
    switch (value)
    {
    case BoardFamily::Stm32H7: return "STM32H7 control board";
    case BoardFamily::NxpS32K3: return "NXP S32K3 control board";
    }
    return "Unknown board";
}
const char* toString(const AnalogFrontend value) noexcept
{
    switch (value)
    {
    case AnalogFrontend::OnChipAdc: return "On-chip ADC";
    case AnalogFrontend::IsolatedAdc: return "Isolated sigma-delta ADC";
    }
    return "Unknown frontend";
}
const char* toString(const TemperatureSensor value) noexcept
{
    switch (value)
    {
    case TemperatureSensor::Ntc10K: return "10 kOhm NTC";
    case TemperatureSensor::Pt100: return "PT100 RTD";
    }
    return "Unknown sensor";
}
const char* toString(const CoolingMode value) noexcept
{
    switch (value)
    {
    case CoolingMode::Quiet: return "Quiet cooling";
    case CoolingMode::Performance: return "Performance cooling";
    }
    return "Unknown cooling mode";
}
} // namespace simulator

