/**
 * @file
 * @brief Embedded Facade example coordinating a safe firmware-update workflow.
 */

#include "catalog/PatternDemo.hpp"

#include <cstdint>
#include <numeric>
#include <sstream>
#include <vector>

namespace catalog::structural
{
namespace
{
class IUpdateSafety
{
public:
    virtual ~IUpdateSafety() = default;
    virtual bool permitsUpdate(bool ignition, bool contactors) const noexcept = 0;
};
class IImageStorage
{
public:
    virtual ~IImageStorage() = default;
    virtual bool eraseInactiveSlot() noexcept = 0;
    virtual bool program(const std::vector<std::uint8_t>& image) noexcept = 0;
};
class IBootSlotSelector
{
public:
    virtual ~IBootSlotSelector() = default;
    virtual void selectInactiveSlot() noexcept = 0;
};

class SafetyInterlock final : public IUpdateSafety
{
public:
    bool permitsUpdate(const bool ignition, const bool contactors) const noexcept override
    {
        return !ignition && !contactors;
    }
};
class FlashMemory final : public IImageStorage
{
public:
    bool eraseInactiveSlot() noexcept override { erased_ = true; return true; }
    bool program(const std::vector<std::uint8_t>& image) noexcept override
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
class BootConfiguration final : public IBootSlotSelector
{
public:
    void selectInactiveSlot() noexcept override { selected_ = true; }
    bool selected() const noexcept { return selected_; }
private:
    bool selected_{false};
};
// SRP: the Facade owns update sequencing only. DIP/ISP: it depends on three small
// capability interfaces, so target flash and boot-control drivers are replaceable.
class FirmwareUpdateFacade
{
public:
    FirmwareUpdateFacade(IUpdateSafety& safety, IImageStorage& flash,
                         IBootSlotSelector& boot) noexcept
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
    IUpdateSafety& safety_;
    IImageStorage& flash_;
    IBootSlotSelector& boot_;
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
