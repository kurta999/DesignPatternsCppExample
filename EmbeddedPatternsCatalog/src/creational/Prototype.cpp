/**
 * @file
 * @brief Embedded Prototype example cloning qualified pressure-channel calibration.
 */

#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <memory>
#include <sstream>

namespace catalog::creational
{
namespace
{
class IChannelProfile
{
public:
    virtual ~IChannelProfile() = default;
    virtual std::unique_ptr<IChannelProfile> clone() const = 0;
    virtual void assignChannel(std::uint8_t channel) noexcept = 0;
    virtual std::uint8_t channel() const noexcept = 0;
};
class PressureChannelProfile final : public IChannelProfile
{
public:
    PressureChannelProfile(const float scale, const float offset, const std::uint16_t samples) noexcept
        : scale_{scale}, offset_{offset}, filterSamples_{samples} {}
    std::unique_ptr<IChannelProfile> clone() const override
    {
        return std::make_unique<PressureChannelProfile>(*this);
    }
    void assignChannel(const std::uint8_t value) noexcept override { channel_ = value; }
    std::uint8_t channel() const noexcept override { return channel_; }
private:
    float scale_;
    float offset_;
    std::uint16_t filterSamples_;
    std::uint8_t channel_{0U};
};
}
DemoResult runPrototype()
{
    const PressureChannelProfile qualifiedTemplate{0.0025F, -1.0F, 16U};
    auto leftBrake = qualifiedTemplate.clone();
    auto rightBrake = qualifiedTemplate.clone();
    leftBrake->assignChannel(2U);
    rightBrake->assignChannel(3U);
    require(leftBrake->channel() == 2U && rightBrake->channel() == 3U,
            "clones need independent identity");
    require(qualifiedTemplate.channel() == 0U, "prototype must stay unchanged");
    std::ostringstream text;
    text << "Cloned qualified pressure calibration for ADC channels 2 and 3.";
    return {"Creational", "Prototype", text.str()};
}
}
