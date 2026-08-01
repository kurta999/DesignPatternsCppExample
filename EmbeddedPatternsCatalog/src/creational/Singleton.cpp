/**
 * @file
 * @brief Embedded Singleton example for a uniquely owned hardware watchdog.
 */

#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <sstream>

namespace catalog::creational
{
namespace
{
enum class CriticalTask : std::uint8_t { MotorControl = 0x01U, SafetyMonitor = 0x02U };

// ISP/DIP: the supervisor needs only the ability to refresh hardware. It does not
// know registers, keys, or which MCU watchdog peripheral implements that action.
class IWatchdogHardware
{
public:
    virtual ~IWatchdogHardware() = default;
    virtual void refresh() noexcept = 0;
};

class RecordingWatchdogHardware final : public IWatchdogHardware
{
public:
    void refresh() noexcept override { ++refreshCount_; }
    std::uint32_t refreshCount() const noexcept { return refreshCount_; }
private:
    std::uint32_t refreshCount_{0U};
};

// SRP: this class decides whether task health permits a refresh; IWatchdogHardware
// performs the I/O. One watchdog owner is legitimate, but ordinary services should
// still prefer explicit construction over hidden global state.
class WatchdogSupervisor final
{
public:
    static WatchdogSupervisor& instance(IWatchdogHardware& hardware) noexcept
    {
        static WatchdogSupervisor value{hardware};
        return value;
    }
    WatchdogSupervisor(const WatchdogSupervisor&) = delete;
    WatchdogSupervisor& operator=(const WatchdogSupervisor&) = delete;
    void reportAlive(const CriticalTask task) noexcept { aliveMask_ |= static_cast<std::uint8_t>(task); }
    bool refreshIfAllTasksAreHealthy() noexcept
    {
        if (aliveMask_ != 0x03U) return false;
        aliveMask_ = 0U;
        hardware_.refresh();
        return true;
    }
private:
    explicit WatchdogSupervisor(IWatchdogHardware& hardware) noexcept : hardware_{hardware} {}
    IWatchdogHardware& hardware_;
    std::uint8_t aliveMask_{0U};
};
}
DemoResult runSingleton()
{
    // The dependency has static lifetime because the Singleton stores a reference.
    static RecordingWatchdogHardware hardware;
    auto& watchdog = WatchdogSupervisor::instance(hardware);
    auto& sameWatchdog = WatchdogSupervisor::instance(hardware);
    require(&watchdog == &sameWatchdog, "watchdog must have one owner");
    watchdog.reportAlive(CriticalTask::MotorControl);
    require(!watchdog.refreshIfAllTasksAreHealthy(), "must wait for every critical task");
    watchdog.reportAlive(CriticalTask::SafetyMonitor);
    require(watchdog.refreshIfAllTasksAreHealthy(), "healthy tasks should permit refresh");
    std::ostringstream text;
    text << "The sole watchdog owner refreshed after both heartbeats (count="
         << hardware.refreshCount() << ").";
    return {"Creational", "Singleton", text.str()};
}
}
