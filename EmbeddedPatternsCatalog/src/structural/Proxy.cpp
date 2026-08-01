#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace catalog::structural
{
namespace
{
class CanBus
{
public:
    void transmitSpeed(const std::uint8_t node, const std::int16_t rpm)
    {
        transmissions_.push_back({node, rpm});
    }
    std::size_t count() const noexcept { return transmissions_.size(); }
private:
    struct Transmission { std::uint8_t node; std::int16_t rpm; };
    std::vector<Transmission> transmissions_{};
};
class IMotorDrive
{
public:
    virtual ~IMotorDrive() = default;
    virtual void setSpeedRpm(std::int16_t rpm) = 0;
    virtual std::optional<std::int16_t> commandedSpeedRpm() const noexcept = 0;
};
// Represents a remote drive locally while enforcing limits and caching commands.
class CanMotorDriveProxy final : public IMotorDrive
{
public:
    CanMotorDriveProxy(CanBus& bus, const std::uint8_t node) noexcept : bus_{bus}, node_{node} {}
    void setSpeedRpm(const std::int16_t rpm) override
    {
        if (rpm < 0 || rpm > 6'000) throw std::out_of_range("unsafe pump speed");
        if (cached_ == rpm) return;
        bus_.transmitSpeed(node_, rpm);
        cached_ = rpm;
    }
    std::optional<std::int16_t> commandedSpeedRpm() const noexcept override { return cached_; }
private:
    CanBus& bus_;
    std::uint8_t node_;
    std::optional<std::int16_t> cached_{};
};
}
DemoResult runProxy()
{
    CanBus bus;
    CanMotorDriveProxy pump{bus, 0x2AU};
    pump.setSpeedRpm(3'200);
    pump.setSpeedRpm(3'200);
    require(bus.count() == 1U && pump.commandedSpeedRpm() == 3'200,
            "proxy did not cache duplicate command");
    std::ostringstream text;
    text << "Validated and cached 3200 rpm for remote CAN node 0x2A; sent " << bus.count() << " frame.";
    return {"Structural", "Proxy", text.str()};
}
}

