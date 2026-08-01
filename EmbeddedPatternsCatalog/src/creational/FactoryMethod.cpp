#include "catalog/PatternDemo.hpp"

#include <sstream>
#include <string_view>

namespace catalog::creational
{
namespace
{
class ITemperatureProbe
{
public:
    virtual ~ITemperatureProbe() = default;
    virtual float readCelsius() const noexcept = 0;
    virtual std::string_view type() const noexcept = 0;
};
class Pt100Probe final : public ITemperatureProbe
{
public:
    float readCelsius() const noexcept override { return 84.5F; }
    std::string_view type() const noexcept override { return "PT100 RTD"; }
};
class KTypeProbe final : public ITemperatureProbe
{
public:
    float readCelsius() const noexcept override { return 612.0F; }
    std::string_view type() const noexcept override { return "K-type thermocouple"; }
};
class TemperatureMonitor
{
public:
    virtual ~TemperatureMonitor() = default;
    bool isOverLimit() const noexcept { return probe().readCelsius() > shutdownLimitC(); }
    std::string_view probeType() const noexcept { return probe().type(); }
protected:
    // The stable algorithm calls this Factory Method; subclasses select a probe.
    virtual const ITemperatureProbe& probe() const noexcept = 0;
    virtual float shutdownLimitC() const noexcept = 0;
};
class MotorWindingMonitor final : public TemperatureMonitor
{
protected:
    const ITemperatureProbe& probe() const noexcept override { return probe_; }
    float shutdownLimitC() const noexcept override { return 150.0F; }
private:
    Pt100Probe probe_{};
};
class ExhaustMonitor final : public TemperatureMonitor
{
protected:
    const ITemperatureProbe& probe() const noexcept override { return probe_; }
    float shutdownLimitC() const noexcept override { return 550.0F; }
private:
    KTypeProbe probe_{};
};
}
DemoResult runFactoryMethod()
{
    const MotorWindingMonitor motor;
    const ExhaustMonitor exhaust;
    require(!motor.isOverLimit() && exhaust.isOverLimit(), "probe-specific limits failed");
    std::ostringstream text;
    text << motor.probeType() << " handles windings; " << exhaust.probeType()
         << " handles the hotter exhaust range.";
    return {"Creational", "Factory Method", text.str()};
}
}

