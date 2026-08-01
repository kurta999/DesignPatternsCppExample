#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <sstream>

namespace catalog::creational
{
namespace
{
enum class CriticalTask : std::uint8_t { MotorControl = 0x01U, SafetyMonitor = 0x02U };
// One watchdog owner is legitimate. Ordinary services should usually use explicit
// construction and injection instead of hidden global state.
class WatchdogSupervisor final
{
public:
    static WatchdogSupervisor& instance() noexcept
    {
        static WatchdogSupervisor value;
        return value;
    }
    WatchdogSupervisor(const WatchdogSupervisor&) = delete;
    WatchdogSupervisor& operator=(const WatchdogSupervisor&) = delete;
    void reportAlive(const CriticalTask task) noexcept { aliveMask_ |= static_cast<std::uint8_t>(task); }
    bool refreshHardwareWatchdog() noexcept
    {
        if (aliveMask_ != 0x03U) return false;
        aliveMask_ = 0U;
        ++refreshCount_;
        return true;
    }
    std::uint32_t refreshCount() const noexcept { return refreshCount_; }
private:
    WatchdogSupervisor() = default;
    std::uint8_t aliveMask_{0U};
    std::uint32_t refreshCount_{0U};
};
}
DemoResult runSingleton()
{
    auto& watchdog = WatchdogSupervisor::instance();
    auto& sameWatchdog = WatchdogSupervisor::instance();
    require(&watchdog == &sameWatchdog, "watchdog must have one owner");
    watchdog.reportAlive(CriticalTask::MotorControl);
    require(!watchdog.refreshHardwareWatchdog(), "must wait for every critical task");
    watchdog.reportAlive(CriticalTask::SafetyMonitor);
    require(watchdog.refreshHardwareWatchdog(), "healthy tasks should permit refresh");
    std::ostringstream text;
    text << "The sole watchdog owner refreshed after both heartbeats (count="
         << watchdog.refreshCount() << ").";
    return {"Creational", "Singleton", text.str()};
}
}

