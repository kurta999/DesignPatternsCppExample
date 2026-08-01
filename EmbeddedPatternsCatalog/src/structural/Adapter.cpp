/**
 * @file
 * @brief Embedded Adapter example converting legacy ADC counts to physical voltage.
 */

#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <sstream>

namespace catalog::structural
{
namespace
{
class LegacyAdcDma
{
public:
    std::uint16_t latestCounts(const std::uint8_t channel) const noexcept
    {
        return channel == 7U ? 43'350U : 0U;
    }
};
class IVoltageSensor
{
public:
    virtual ~IVoltageSensor() = default;
    virtual float readVolts() const noexcept = 0;
};
// Adapts an unchangeable legacy counts API to a physical-unit interface.
class AdcVoltageAdapter final : public IVoltageSensor
{
public:
    AdcVoltageAdapter(const LegacyAdcDma& adc, const std::uint8_t channel,
                      const float referenceV, const float divider) noexcept
        : adc_{adc}, channel_{channel}, referenceV_{referenceV}, divider_{divider} {}
    float readVolts() const noexcept override
    {
        return (static_cast<float>(adc_.latestCounts(channel_)) / 65'535.0F) *
               referenceV_ * divider_;
    }
private:
    const LegacyAdcDma& adc_;
    std::uint8_t channel_;
    float referenceV_;
    float divider_;
};
}
DemoResult runAdapter()
{
    const LegacyAdcDma adc;
    const AdcVoltageAdapter supply{adc, 7U, 3.3F, 11.0F};
    const float volts = supply.readVolts();
    require(volts > 23.9F && volts < 24.1F, "expected a 24 V supply");
    std::ostringstream text;
    text << "Adapted raw DMA counts into " << volts << " V.";
    return {"Structural", "Adapter", text.str()};
}
}
