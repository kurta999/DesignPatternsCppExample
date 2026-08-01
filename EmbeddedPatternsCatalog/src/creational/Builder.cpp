#include "catalog/PatternDemo.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>

namespace catalog::creational
{
namespace
{
struct CanFdFrame
{
    std::uint32_t identifier{0U};
    bool extendedIdentifier{false};
    bool bitRateSwitch{false};
    std::array<std::uint8_t, 64> payload{};
    std::size_t payloadLength{0U};
};
// Keeps validation and byte order out of application call sites.
class CanFdFrameBuilder
{
public:
    CanFdFrameBuilder& withExtendedIdentifier(const std::uint32_t value)
    {
        if (value > 0x1FFFFFFFU) throw std::invalid_argument("CAN identifier out of range");
        frame_.identifier = value;
        frame_.extendedIdentifier = true;
        return *this;
    }
    CanFdFrameBuilder& withBitRateSwitch() noexcept { frame_.bitRateSwitch = true; return *this; }
    CanFdFrameBuilder& appendU16BigEndian(const std::uint16_t value)
    {
        ensureSpace(2U);
        frame_.payload[frame_.payloadLength++] = static_cast<std::uint8_t>(value >> 8U);
        frame_.payload[frame_.payloadLength++] = static_cast<std::uint8_t>(value & 0xFFU);
        return *this;
    }
    CanFdFrameBuilder& appendI16BigEndian(const std::int16_t value)
    {
        return appendU16BigEndian(static_cast<std::uint16_t>(value));
    }
    CanFdFrame build() const
    {
        if (frame_.payloadLength == 0U) throw std::logic_error("empty CAN frame");
        return frame_;
    }
private:
    void ensureSpace(const std::size_t bytes) const
    {
        if (frame_.payloadLength + bytes > frame_.payload.size())
            throw std::length_error("CAN FD payload exceeds 64 bytes");
    }
    CanFdFrame frame_{};
};
}
DemoResult runBuilder()
{
    const auto frame = CanFdFrameBuilder{}.withExtendedIdentifier(0x18FF50E5U)
                           .withBitRateSwitch().appendU16BigEndian(3'984U)
                           .appendI16BigEndian(-127).build();
    require(frame.payloadLength == 4U, "BMS payload length is wrong");
    require(frame.payload[0] == 0x0FU && frame.payload[1] == 0x90U,
            "pack voltage byte order is wrong");
    std::ostringstream text;
    text << "Built CAN-FD frame 0x" << std::hex << frame.identifier
         << " with a validated BMS payload.";
    return {"Creational", "Builder", text.str()};
}
}

