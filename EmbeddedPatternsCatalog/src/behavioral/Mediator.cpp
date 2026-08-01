#include "catalog/PatternDemo.hpp"

namespace catalog::behavioral
{
namespace
{
enum class Event { PilotConnected, PilotDisconnected, ThermalSafe, ThermalHigh };
class IMediator
{
public:
    virtual ~IMediator() = default;
    virtual void notify(Event event) noexcept = 0;
};
class ChargeContactor
{
public:
    void close() noexcept { closed_ = true; }
    void open() noexcept { closed_ = false; }
    bool isClosed() const noexcept { return closed_; }
private:
    bool closed_{false};
};
class PilotInput
{
public:
    explicit PilotInput(IMediator& mediator) noexcept : mediator_{mediator} {}
    void setConnected(const bool connected) noexcept
    { mediator_.notify(connected ? Event::PilotConnected : Event::PilotDisconnected); }
private:
    IMediator& mediator_;
};
class ThermalMonitor
{
public:
    explicit ThermalMonitor(IMediator& mediator) noexcept : mediator_{mediator} {}
    void sample(const float temperatureC) noexcept
    { mediator_.notify(temperatureC < 50.0F ? Event::ThermalSafe : Event::ThermalHigh); }
private:
    IMediator& mediator_;
};
// Components do not reference one another; this coordinator owns their policy.
class ChargingCoordinator final : public IMediator
{
public:
    explicit ChargingCoordinator(ChargeContactor& contactor) noexcept : contactor_{contactor} {}
    void notify(const Event event) noexcept override
    {
        switch (event)
        {
        case Event::PilotConnected: pilot_ = true; break;
        case Event::PilotDisconnected: pilot_ = false; break;
        case Event::ThermalSafe: thermalSafe_ = true; break;
        case Event::ThermalHigh: thermalSafe_ = false; break;
        }
        if (pilot_ && thermalSafe_) contactor_.close(); else contactor_.open();
    }
private:
    ChargeContactor& contactor_;
    bool pilot_{false};
    bool thermalSafe_{false};
};
}
DemoResult runMediator()
{
    ChargeContactor contactor;
    ChargingCoordinator coordinator{contactor};
    PilotInput pilot{coordinator};
    ThermalMonitor thermal{coordinator};
    thermal.sample(32.0F);
    pilot.setConnected(true);
    require(contactor.isClosed(), "safe charging did not start");
    thermal.sample(55.0F);
    require(!contactor.isClosed(), "thermal trip did not stop charging");
    return {"Behavioral", "Mediator",
            "A coordinator coupled pilot and thermal events without subsystem dependencies."};
}
}

