#include "catalog/PatternDemo.hpp"

#include <cmath>

namespace catalog::behavioral
{
namespace
{
class MotorCalibration
{
public:
    class Memento
    {
        friend class MotorCalibration;
        Memento(const float kp, const float ki, const float offset) noexcept
            : kp_{kp}, ki_{ki}, offset_{offset} {}
        float kp_;
        float ki_;
        float offset_;
    };
    Memento save() const noexcept { return {kp_, ki_, offset_}; }
    void restore(const Memento& value) noexcept
    { kp_ = value.kp_; ki_ = value.ki_; offset_ = value.offset_; }
    void apply(const float kp, const float ki, const float offset) noexcept
    { kp_ = kp; ki_ = ki; offset_ = offset; }
    bool passesCheck() const noexcept
    { return kp_ > 0.0F && kp_ <= 2.0F && ki_ >= 0.0F && ki_ <= 0.5F && std::fabs(offset_) <= 15.0F; }
    float kp() const noexcept { return kp_; }
private:
    float kp_{0.8F};
    float ki_{0.12F};
    float offset_{1.5F};
};
}
DemoResult runMemento()
{
    MotorCalibration calibration;
    const auto knownGood = calibration.save();
    calibration.apply(4.5F, 0.12F, 1.5F);
    require(!calibration.passesCheck(), "unsafe gain passed");
    calibration.restore(knownGood);
    require(calibration.passesCheck() && std::fabs(calibration.kp() - 0.8F) < 0.01F,
            "rollback failed");
    return {"Behavioral", "Memento",
            "Rolled back an invalid motor gain to the known-good calibration."};
}
}

