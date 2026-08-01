#include "catalog/PatternDemo.hpp"

#include <functional>
#include <vector>

namespace catalog::structural
{
namespace
{
class IPowerNode
{
public:
    virtual ~IPowerNode() = default;
    virtual float currentA() const noexcept = 0;
    virtual bool isEnabled() const noexcept = 0;
    virtual void shutdown() noexcept = 0;
};
class ElectronicLoad final : public IPowerNode
{
public:
    explicit ElectronicLoad(const float current) noexcept : current_{current} {}
    float currentA() const noexcept override { return enabled_ ? current_ : 0.0F; }
    bool isEnabled() const noexcept override { return enabled_; }
    void shutdown() noexcept override { enabled_ = false; }
private:
    float current_;
    bool enabled_{true};
};
// Branches and leaves share one interface, enabling recursive power operations.
class PowerBranch final : public IPowerNode
{
public:
    void add(IPowerNode& child) { children_.push_back(child); }
    float currentA() const noexcept override
    {
        float total = 0.0F;
        for (const auto& child : children_) total += child.get().currentA();
        return total;
    }
    bool isEnabled() const noexcept override
    {
        for (const auto& child : children_) if (child.get().isEnabled()) return true;
        return false;
    }
    void shutdown() noexcept override
    {
        for (auto& child : children_) child.get().shutdown();
    }
private:
    std::vector<std::reference_wrapper<IPowerNode>> children_;
};
}
DemoResult runComposite()
{
    ElectronicLoad sensors{0.18F}, communication{0.42F}, pump{3.6F};
    PowerBranch electronics;
    electronics.add(sensors);
    electronics.add(communication);
    PowerBranch vehicle;
    vehicle.add(electronics);
    vehicle.add(pump);
    require(vehicle.currentA() > 4.19F && vehicle.currentA() < 4.21F, "power-tree sum failed");
    vehicle.shutdown();
    require(!vehicle.isEnabled() && vehicle.currentA() == 0.0F, "recursive shutdown failed");
    return {"Structural", "Composite",
            "One command recursively shut down the electronics branch and coolant pump."};
}
}

