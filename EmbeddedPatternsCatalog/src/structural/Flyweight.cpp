#include "catalog/PatternDemo.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace catalog::structural
{
namespace
{
// Immutable intrinsic calibration data is shared by identical channels.
class ThermistorCurve final
{
public:
    ThermistorCurve(std::string part, const float beta) noexcept
        : part_{std::move(part)}, beta_{beta} {}
    float celsius(const float resistance) const noexcept
    {
        const float inverseK = (1.0F / 298.15F) + (std::log(resistance / 10'000.0F) / beta_);
        return (1.0F / inverseK) - 273.15F;
    }
private:
    std::string part_;
    float beta_;
};
class CurveRepository
{
public:
    std::shared_ptr<const ThermistorCurve> curveFor(const std::string& part)
    {
        const auto found = curves_.find(part);
        if (found != curves_.end()) return found->second;
        auto curve = std::make_shared<const ThermistorCurve>(part, 3'950.0F);
        curves_.emplace(part, curve);
        return curve;
    }
private:
    std::unordered_map<std::string, std::shared_ptr<const ThermistorCurve>> curves_;
};
class ThermistorChannel
{
public:
    ThermistorChannel(const std::uint8_t channel, std::shared_ptr<const ThermistorCurve> curve) noexcept
        : channel_{channel}, curve_{std::move(curve)} {}
    float temperatureC(const float resistance) const noexcept { return curve_->celsius(resistance); }
    const ThermistorCurve* curveAddress() const noexcept { return curve_.get(); }
    std::uint8_t channel() const noexcept { return channel_; }
private:
    std::uint8_t channel_;
    std::shared_ptr<const ThermistorCurve> curve_;
};
}
DemoResult runFlyweight()
{
    CurveRepository repository;
    ThermistorChannel first{1U, repository.curveFor("NTC-10K-3950")};
    ThermistorChannel second{2U, repository.curveFor("NTC-10K-3950")};
    require(first.curveAddress() == second.curveAddress(), "curve was not shared");
    require(std::fabs(first.temperatureC(10'000.0F) - 25.0F) < 0.1F, "curve is wrong");
    std::ostringstream text;
    text << "ADC channels " << static_cast<int>(first.channel()) << " and "
         << static_cast<int>(second.channel()) << " share one thermistor curve.";
    return {"Structural", "Flyweight", text.str()};
}
}

