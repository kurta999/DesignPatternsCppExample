/**
 * @file
 * @brief Embedded Bridge example separating thermal alarms from their output transport.
 */

#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <sstream>
#include <string_view>

namespace catalog::structural
{
namespace
{
class IAlarmOutput
{
public:
    virtual ~IAlarmOutput() = default;
    virtual void activate(std::uint16_t code) = 0;
    virtual std::string_view medium() const noexcept = 0;
};
class CanAlarmOutput final : public IAlarmOutput
{
public:
    void activate(const std::uint16_t code) override { code_ = code; }
    std::string_view medium() const noexcept override { return "CAN diagnostic frame"; }
    std::uint16_t code() const noexcept { return code_; }
private:
    std::uint16_t code_{0U};
};
class BuzzerOutput final : public IAlarmOutput
{
public:
    void activate(const std::uint16_t code) override { pulses_ = code >= 0x8000U ? 5U : 2U; }
    std::string_view medium() const noexcept override { return "local buzzer"; }
    std::uint8_t pulses() const noexcept { return pulses_; }
private:
    std::uint8_t pulses_{0U};
};
// Alarm semantics and physical transport can evolve independently.
class EquipmentAlarm
{
public:
    explicit EquipmentAlarm(IAlarmOutput& output) noexcept : output_{output} {}
    virtual ~EquipmentAlarm() = default;
    virtual void raise() = 0;
protected:
    IAlarmOutput& output_;
};
class ThermalAlarm final : public EquipmentAlarm
{
public:
    using EquipmentAlarm::EquipmentAlarm;
    void raise() override { output_.activate(0x8102U); }
};
}
DemoResult runBridge()
{
    CanAlarmOutput can;
    BuzzerOutput buzzer;
    ThermalAlarm remote{can};
    ThermalAlarm local{buzzer};
    remote.raise();
    local.raise();
    require(can.code() == 0x8102U && buzzer.pulses() == 5U, "alarm outputs disagree");
    std::ostringstream text;
    text << "One thermal alarm drives a " << can.medium() << " or " << buzzer.medium() << '.';
    return {"Structural", "Bridge", text.str()};
}
}
