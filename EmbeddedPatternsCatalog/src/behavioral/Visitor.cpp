/**
 * @file
 * @brief Embedded Visitor example auditing hardware and calculating its power budget.
 */

#include "catalog/PatternDemo.hpp"

#include <cstddef>
#include <functional>
#include <sstream>
#include <vector>

namespace catalog::behavioral
{
namespace
{
class TemperatureSensor;
class SafetyRelay;
class MotorDrive;
class IHardwareVisitor
{
public:
    virtual ~IHardwareVisitor() = default;
    virtual void visit(const TemperatureSensor&) noexcept = 0;
    virtual void visit(const SafetyRelay&) noexcept = 0;
    virtual void visit(const MotorDrive&) noexcept = 0;
};
class IHardwareElement
{
public:
    virtual ~IHardwareElement() = default;
    virtual void accept(IHardwareVisitor&) const noexcept = 0;
};
class TemperatureSensor final : public IHardwareElement
{
public:
    explicit TemperatureSensor(const float watts) noexcept : watts_{watts} {}
    void accept(IHardwareVisitor& visitor) const noexcept override { visitor.visit(*this); }
    float watts() const noexcept { return watts_; }
private:
    float watts_;
};
class SafetyRelay final : public IHardwareElement
{
public:
    SafetyRelay(const float watts, const bool feedback) noexcept : watts_{watts}, feedback_{feedback} {}
    void accept(IHardwareVisitor& visitor) const noexcept override { visitor.visit(*this); }
    float watts() const noexcept { return watts_; }
    bool hasFeedback() const noexcept { return feedback_; }
private:
    float watts_;
    bool feedback_;
};
class MotorDrive final : public IHardwareElement
{
public:
    MotorDrive(const float watts, const bool sto) noexcept : watts_{watts}, sto_{sto} {}
    void accept(IHardwareVisitor& visitor) const noexcept override { visitor.visit(*this); }
    float watts() const noexcept { return watts_; }
    bool hasSto() const noexcept { return sto_; }
private:
    float watts_;
    bool sto_;
};
class PowerBudgetVisitor final : public IHardwareVisitor
{
public:
    void visit(const TemperatureSensor& v) noexcept override { total_ += v.watts(); }
    void visit(const SafetyRelay& v) noexcept override { total_ += v.watts(); }
    void visit(const MotorDrive& v) noexcept override { total_ += v.watts(); }
    float total() const noexcept { return total_; }
private:
    float total_{0.0F};
};
class SafetyAuditVisitor final : public IHardwareVisitor
{
public:
    void visit(const TemperatureSensor&) noexcept override {}
    void visit(const SafetyRelay& v) noexcept override { if (!v.hasFeedback()) ++issues_; }
    void visit(const MotorDrive& v) noexcept override { if (!v.hasSto()) ++issues_; }
    std::size_t issues() const noexcept { return issues_; }
private:
    std::size_t issues_{0U};
};
}
DemoResult runVisitor()
{
    TemperatureSensor sensor{0.08F};
    SafetyRelay relay{1.20F, true};
    MotorDrive drive{18.0F, true};
    const std::vector<std::reference_wrapper<const IHardwareElement>> hardware{sensor, relay, drive};
    PowerBudgetVisitor power;
    SafetyAuditVisitor safety;
    for (const auto& element : hardware)
    {
        element.get().accept(power);
        element.get().accept(safety);
    }
    require(power.total() > 19.27F && power.total() < 19.29F, "power budget failed");
    require(safety.issues() == 0U, "safety audit failed");
    std::ostringstream text;
    text << "Ran power and safety passes; load=" << power.total() << " W, issues=" << safety.issues() << '.';
    return {"Behavioral", "Visitor", text.str()};
}
}
