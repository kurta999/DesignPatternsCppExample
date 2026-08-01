#include "catalog/PatternDemo.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <sstream>
#include <vector>

namespace catalog::behavioral
{
namespace
{
struct CellSample { std::uint8_t cell; float voltageV; };
class ICellObserver
{
public:
    virtual ~ICellObserver() = default;
    virtual void onCellVoltage(const CellSample&) noexcept = 0;
};
class CellMonitor
{
public:
    void attach(ICellObserver& observer) { observers_.push_back(observer); }
    void publish(const CellSample& sample) noexcept
    {
        // Target callbacks must be bounded; slow logging should enqueue work.
        for (auto& observer : observers_) observer.get().onCellVoltage(sample);
    }
private:
    std::vector<std::reference_wrapper<ICellObserver>> observers_{};
};
class ContactorProtection final : public ICellObserver
{
public:
    void onCellVoltage(const CellSample& sample) noexcept override
    { if (sample.voltageV < 2.70F) contactorClosed_ = false; }
    bool contactorClosed() const noexcept { return contactorClosed_; }
private:
    bool contactorClosed_{true};
};
class Dashboard final : public ICellObserver
{
public:
    void onCellVoltage(const CellSample& sample) noexcept override
    { minimum_ = std::min(minimum_, sample.voltageV); }
    float minimum() const noexcept { return minimum_; }
private:
    float minimum_{std::numeric_limits<float>::max()};
};
class FaultRecorder final : public ICellObserver
{
public:
    void onCellVoltage(const CellSample& sample) noexcept override
    { if (sample.voltageV < 2.70F) ++events_; }
    std::size_t events() const noexcept { return events_; }
private:
    std::size_t events_{0U};
};
}
DemoResult runObserver()
{
    CellMonitor monitor;
    ContactorProtection protection;
    Dashboard dashboard;
    FaultRecorder recorder;
    monitor.attach(protection);
    monitor.attach(dashboard);
    monitor.attach(recorder);
    monitor.publish({17U, 2.61F});
    require(!protection.contactorClosed(), "undervoltage did not open contactor");
    require(dashboard.minimum() == 2.61F && recorder.events() == 1U, "observers missed sample");
    std::ostringstream text;
    text << "Cell 17 notified protection, dashboard, and fault recorder.";
    return {"Behavioral", "Observer", text.str()};
}
}

