#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <numeric>
#include <sstream>
#include <vector>

namespace catalog::structural
{
namespace
{
class SafetyInterlock
{
public:
    bool permitsUpdate(const bool ignition, const bool contactors) const noexcept
    {
        return !ignition && !contactors;
    }
};
class FlashMemory
{
public:
    bool eraseInactiveSlot() noexcept { erased_ = true; return true; }
    bool program(const std::vector<std::uint8_t>& image) noexcept
    {
        if (!erased_ || image.empty()) return false;
        checksum_ = std::accumulate(image.begin(), image.end(), std::uint32_t{0U});
        return true;
    }
    std::uint32_t checksum() const noexcept { return checksum_; }
private:
    bool erased_{false};
    std::uint32_t checksum_{0U};
};
class BootConfiguration
{
public:
    void selectInactiveSlot() noexcept { selected_ = true; }
    bool selected() const noexcept { return selected_; }
private:
    bool selected_{false};
};
// Provides one ordered, safe update operation instead of exposing subsystems.
class FirmwareUpdateFacade
{
public:
    FirmwareUpdateFacade(SafetyInterlock& safety, FlashMemory& flash,
                         BootConfiguration& boot) noexcept
        : safety_{safety}, flash_{flash}, boot_{boot} {}
    bool install(const std::vector<std::uint8_t>& image, const bool ignition,
                 const bool contactors) noexcept
    {
        if (!safety_.permitsUpdate(ignition, contactors) ||
            !flash_.eraseInactiveSlot() || !flash_.program(image)) return false;
        boot_.selectInactiveSlot();
        return true;
    }
private:
    SafetyInterlock& safety_;
    FlashMemory& flash_;
    BootConfiguration& boot_;
};
}
DemoResult runFacade()
{
    SafetyInterlock safety;
    FlashMemory flash;
    BootConfiguration boot;
    FirmwareUpdateFacade updater{safety, flash, boot};
    const std::vector<std::uint8_t> image{0x45U, 0x43U, 0x55U, 0x01U};
    require(!updater.install(image, true, false), "update allowed with ignition on");
    require(updater.install(image, false, false) && boot.selected(), "safe update failed");
    std::ostringstream text;
    text << "Coordinated interlock, flash, and boot slot (checksum=" << flash.checksum() << ").";
    return {"Structural", "Facade", text.str()};
}
}

