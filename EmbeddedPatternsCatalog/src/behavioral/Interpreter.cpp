/**
 * @file
 * @brief Embedded Interpreter example composing a machine-start interlock rule.
 */

#include "catalog/PatternDemo.hpp"

#include <memory>
#include <utility>

namespace catalog::behavioral
{
namespace
{
struct StartValues { float pressureBar; float temperatureC; bool guardClosed; };
class IExpression
{
public:
    virtual ~IExpression() = default;
    virtual bool evaluate(const StartValues&) const noexcept = 0;
};
class PressureAbove final : public IExpression
{
public:
    explicit PressureAbove(const float threshold) noexcept : threshold_{threshold} {}
    bool evaluate(const StartValues& v) const noexcept override { return v.pressureBar > threshold_; }
private:
    float threshold_;
};
class TemperatureBelow final : public IExpression
{
public:
    explicit TemperatureBelow(const float threshold) noexcept : threshold_{threshold} {}
    bool evaluate(const StartValues& v) const noexcept override { return v.temperatureC < threshold_; }
private:
    float threshold_;
};
class GuardClosed final : public IExpression
{
public:
    bool evaluate(const StartValues& v) const noexcept override { return v.guardClosed; }
};
class AndExpression final : public IExpression
{
public:
    AndExpression(std::unique_ptr<IExpression> left, std::unique_ptr<IExpression> right) noexcept
        : left_{std::move(left)}, right_{std::move(right)} {}
    bool evaluate(const StartValues& v) const noexcept override
    { return left_->evaluate(v) && right_->evaluate(v); }
private:
    std::unique_ptr<IExpression> left_;
    std::unique_ptr<IExpression> right_;
};
}
DemoResult runInterpreter()
{
    auto pressureAndTemperature = std::make_unique<AndExpression>(
        std::make_unique<PressureAbove>(2.5F), std::make_unique<TemperatureBelow>(80.0F));
    const AndExpression startRule{std::move(pressureAndTemperature), std::make_unique<GuardClosed>()};
    require(startRule.evaluate({3.1F, 62.0F, true}), "safe start rule failed");
    require(!startRule.evaluate({3.1F, 62.0F, false}), "open guard was accepted");
    return {"Behavioral", "Interpreter",
            "Evaluated a composable start rule over pressure, temperature, and guard state."};
}
}
