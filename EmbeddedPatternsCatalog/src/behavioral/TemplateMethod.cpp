/**
 * @file
 * @brief Embedded Template Method example reusing a safe sensor self-test sequence.
 */

#include "catalog/PatternDemo.hpp"

#include <cmath>
#include <sstream>
#include <string_view>

namespace catalog::behavioral
{
namespace
{
struct TestResult { bool passed; float reading; };
class SensorSelfTest
{
public:
    virtual ~SensorSelfTest() = default;
    TestResult run() noexcept
    {
        // The safety-relevant sequence is fixed; only sensor-specific steps vary.
        enableStimulus();
        waitForSettling();
        const float reading = readReference();
        const bool passed = std::fabs(reading - expected()) <= tolerance();
        disableStimulus();
        return {passed, reading};
    }
    virtual std::string_view name() const noexcept = 0;
protected:
    virtual void enableStimulus() noexcept { stimulus_ = true; }
    virtual void waitForSettling() noexcept { settled_ = stimulus_; }
    virtual float readReference() const noexcept = 0;
    virtual float expected() const noexcept = 0;
    virtual float tolerance() const noexcept = 0;
    virtual void disableStimulus() noexcept { stimulus_ = false; }
    bool settled() const noexcept { return settled_; }
private:
    bool stimulus_{false};
    bool settled_{false};
};
class HallSensorTest final : public SensorSelfTest
{
public:
    std::string_view name() const noexcept override { return "Hall current sensor"; }
protected:
    float readReference() const noexcept override { return settled() ? 1.648F : 0.0F; }
    float expected() const noexcept override { return 1.650F; }
    float tolerance() const noexcept override { return 0.010F; }
};
class ResolverTest final : public SensorSelfTest
{
public:
    std::string_view name() const noexcept override { return "Motor resolver"; }
protected:
    float readReference() const noexcept override { return settled() ? 2.49F : 0.0F; }
    float expected() const noexcept override { return 2.50F; }
    float tolerance() const noexcept override { return 0.05F; }
};
}
DemoResult runTemplateMethod()
{
    HallSensorTest hall;
    ResolverTest resolver;
    require(hall.run().passed && resolver.run().passed, "sensor self-test failed");
    std::ostringstream text;
    text << hall.name() << " and " << resolver.name() << " shared one safe test sequence.";
    return {"Behavioral", "Template Method", text.str()};
}
}
