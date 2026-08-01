#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <sstream>
#include <utility>
#include <vector>

namespace catalog::structural
{
namespace
{
using Bytes = std::vector<std::uint8_t>;
class IFrameSender
{
public:
    virtual ~IFrameSender() = default;
    virtual void send(Bytes payload) = 0;
};
class CanFrameSender final : public IFrameSender
{
public:
    void send(Bytes payload) override { last_ = std::move(payload); }
    const Bytes& lastFrame() const noexcept { return last_; }
private:
    Bytes last_{};
};
class SenderDecorator : public IFrameSender
{
public:
    explicit SenderDecorator(IFrameSender& inner) noexcept : inner_{inner} {}
protected:
    IFrameSender& inner_;
};
class SequenceDecorator final : public SenderDecorator
{
public:
    using SenderDecorator::SenderDecorator;
    void send(Bytes payload) override
    {
        payload.insert(payload.begin(), next_++);
        inner_.send(std::move(payload));
    }
private:
    std::uint8_t next_{0U};
};
class CrcDecorator final : public SenderDecorator
{
public:
    using SenderDecorator::SenderDecorator;
    void send(Bytes payload) override
    {
        std::uint8_t crc = 0xFFU;
        for (const auto byte : payload) crc ^= byte;
        payload.push_back(crc);
        inner_.send(std::move(payload));
    }
};
}
DemoResult runDecorator()
{
    CanFrameSender can;
    CrcDecorator crc{can};
    SequenceDecorator protectedSender{crc};
    protectedSender.send({0x31U, 0x7AU});
    const auto& frame = can.lastFrame();
    require(frame.size() == 4U && frame.front() == 0U, "decorators did not wrap payload");
    require(frame.back() == static_cast<std::uint8_t>(0xFFU ^ 0x00U ^ 0x31U ^ 0x7AU),
            "CRC is wrong");
    std::ostringstream text;
    text << "Added sequence and CRC layers without changing the CAN sender ("
         << frame.size() << " bytes).";
    return {"Structural", "Decorator", text.str()};
}
}

