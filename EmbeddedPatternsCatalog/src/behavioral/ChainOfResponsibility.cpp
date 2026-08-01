/**
 * @file
 * @brief Embedded Chain of Responsibility example prioritizing machine protection faults.
 */

#include "catalog/PatternDemo.hpp"

#include <optional>
#include <string_view>

namespace catalog::behavioral
{
namespace
{
struct Measurements { bool emergency; float temperatureC; float coolantPressureBar; };
class FaultHandler
{
public:
    virtual ~FaultHandler() = default;
    FaultHandler& setNext(FaultHandler& next) noexcept { next_ = &next; return next; }
    std::optional<std::string_view> handle(const Measurements& values) const noexcept
    {
        const auto fault = check(values);
        return (fault.has_value() || next_ == nullptr) ? fault : next_->handle(values);
    }
protected:
    virtual std::optional<std::string_view> check(const Measurements&) const noexcept = 0;
private:
    FaultHandler* next_{nullptr};
};
class EmergencyHandler final : public FaultHandler
{
    std::optional<std::string_view> check(const Measurements& v) const noexcept override
    { return v.emergency ? std::optional<std::string_view>{"Emergency stop"} : std::nullopt; }
};
class ThermalHandler final : public FaultHandler
{
    std::optional<std::string_view> check(const Measurements& v) const noexcept override
    { return v.temperatureC > 100.0F ? std::optional<std::string_view>{"Inverter overtemperature"} : std::nullopt; }
};
class CoolantHandler final : public FaultHandler
{
    std::optional<std::string_view> check(const Measurements& v) const noexcept override
    { return v.coolantPressureBar < 1.0F ? std::optional<std::string_view>{"Low coolant pressure"} : std::nullopt; }
};
}
DemoResult runChainOfResponsibility()
{
    EmergencyHandler emergency;
    ThermalHandler thermal;
    CoolantHandler coolant;
    emergency.setNext(thermal).setNext(coolant);
    const auto fault = emergency.handle({false, 112.0F, 0.4F});
    require(fault == "Inverter overtemperature", "fault priority chain failed");
    return {"Behavioral", "Chain of Responsibility",
            "A protection chain reported overtemperature before lower-priority coolant pressure."};
}
}
