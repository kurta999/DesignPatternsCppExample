#include "catalog/PatternDemo.hpp"

#include <algorithm>
#include <sstream>

namespace catalog::behavioral
{
namespace
{
class IFanStrategy
{
public:
    virtual ~IFanStrategy() = default;
    virtual float dutyPercent(float temperatureC) const noexcept = 0;
};
class QuietStrategy final : public IFanStrategy
{
public:
    float dutyPercent(const float t) const noexcept override
    { return std::clamp((t - 40.0F) * 2.0F, 0.0F, 60.0F); }
};
class MaximumCoolingStrategy final : public IFanStrategy
{
public:
    float dutyPercent(const float t) const noexcept override
    { return std::clamp((t - 30.0F) * 3.0F, 20.0F, 100.0F); }
};
class CoolingController
{
public:
    explicit CoolingController(const IFanStrategy& strategy) noexcept : strategy_{&strategy} {}
    void select(const IFanStrategy& strategy) noexcept { strategy_ = &strategy; }
    float commandDuty(const float temperature) const noexcept { return strategy_->dutyPercent(temperature); }
private:
    const IFanStrategy* strategy_;
};
}
DemoResult runStrategy()
{
    const QuietStrategy quiet;
    const MaximumCoolingStrategy maximum;
    CoolingController controller{quiet};
    const float quietDuty = controller.commandDuty(65.0F);
    controller.select(maximum);
    const float maximumDuty = controller.commandDuty(65.0F);
    require(quietDuty == 50.0F && maximumDuty == 100.0F, "fan strategy selection failed");
    std::ostringstream text;
    text << "At 65 C, policy changed fan duty from " << quietDuty << "% to " << maximumDuty << "%.";
    return {"Behavioral", "Strategy", text.str()};
}
}

