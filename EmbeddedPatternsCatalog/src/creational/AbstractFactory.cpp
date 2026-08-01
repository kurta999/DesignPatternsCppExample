#include "catalog/PatternDemo.hpp"

#include <memory>
#include <sstream>
#include <string_view>

namespace catalog::creational
{
namespace
{
class IAdcDriver
{
public:
    virtual ~IAdcDriver() = default;
    virtual std::string_view peripheral() const noexcept = 0;
    virtual int readSupplyMillivolts() const noexcept = 0;
};
class ICanDriver
{
public:
    virtual ~ICanDriver() = default;
    virtual std::string_view peripheral() const noexcept = 0;
    virtual int nominalBitrate() const noexcept = 0;
};
// Creates a compatible family, preventing an application from mixing drivers
// configured for different MCU board-support packages.
class IBoardSupportFactory
{
public:
    virtual ~IBoardSupportFactory() = default;
    virtual std::unique_ptr<IAdcDriver> createAdc() const = 0;
    virtual std::unique_ptr<ICanDriver> createCan() const = 0;
};
class Stm32Adc final : public IAdcDriver
{
public:
    std::string_view peripheral() const noexcept override { return "STM32 ADC3"; }
    int readSupplyMillivolts() const noexcept override { return 3'298; }
};
class Stm32Can final : public ICanDriver
{
public:
    std::string_view peripheral() const noexcept override { return "STM32 FDCAN1"; }
    int nominalBitrate() const noexcept override { return 500'000; }
};
class Stm32BoardFactory final : public IBoardSupportFactory
{
public:
    std::unique_ptr<IAdcDriver> createAdc() const override { return std::make_unique<Stm32Adc>(); }
    std::unique_ptr<ICanDriver> createCan() const override { return std::make_unique<Stm32Can>(); }
};
class NxpAdc final : public IAdcDriver
{
public:
    std::string_view peripheral() const noexcept override { return "NXP ADC0"; }
    int readSupplyMillivolts() const noexcept override { return 3'301; }
};
class NxpCan final : public ICanDriver
{
public:
    std::string_view peripheral() const noexcept override { return "NXP FlexCAN0"; }
    int nominalBitrate() const noexcept override { return 500'000; }
};
class NxpBoardFactory final : public IBoardSupportFactory
{
public:
    std::unique_ptr<IAdcDriver> createAdc() const override { return std::make_unique<NxpAdc>(); }
    std::unique_ptr<ICanDriver> createCan() const override { return std::make_unique<NxpCan>(); }
};
class PowerBoardDiagnostics
{
public:
    explicit PowerBoardDiagnostics(const IBoardSupportFactory& factory)
        : adc_{factory.createAdc()}, can_{factory.createCan()} {}
    int supplyMillivolts() const noexcept { return adc_->readSupplyMillivolts(); }
    int canBitrate() const noexcept { return can_->nominalBitrate(); }
    std::string_view adcName() const noexcept { return adc_->peripheral(); }
    std::string_view canName() const noexcept { return can_->peripheral(); }
private:
    std::unique_ptr<IAdcDriver> adc_;
    std::unique_ptr<ICanDriver> can_;
};
}
DemoResult runAbstractFactory()
{
    const Stm32BoardFactory productionFactory;
    const NxpBoardFactory alternateFactory;
    const PowerBoardDiagnostics production{productionFactory};
    const PowerBoardDiagnostics alternate{alternateFactory};
    require(production.canBitrate() == alternate.canBitrate(), "CAN bitrate mismatch");
    require(production.supplyMillivolts() >= 3'250 && production.supplyMillivolts() <= 3'350,
            "supply is outside tolerance");
    std::ostringstream text;
    text << production.adcName() << " and " << production.canName()
         << " were selected as one compatible BSP family.";
    return {"Creational", "Abstract Factory", text.str()};
}
}

