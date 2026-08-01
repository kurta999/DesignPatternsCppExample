#pragma once

/**
 * @file PatternDemo.hpp
 * @brief Common result contract and entry points for all 23 GoF demonstrations.
 */

#include <stdexcept>
#include <string>
#include <string_view>

namespace catalog
{
/** @addtogroup pattern_catalog
 *  @{
 */
/** @brief Self-describing result returned by one executable pattern example. */
struct DemoResult
{
    std::string_view category; ///< Creational, Structural, or Behavioral.
    std::string_view pattern; ///< Gang of Four pattern name.
    std::string outcome; ///< Scenario-specific result text.
};

/** @brief Uniform function pointer used by the catalog runner. */
using DemoFunction = DemoResult (*)();

/**
 * @brief Enforce one executable demo invariant.
 * @throws std::logic_error with @p message when @p condition is false.
 */
inline void require(const bool condition, const std::string_view message)
{
    if (!condition) throw std::logic_error(std::string{message});
}

namespace creational
{
/** @name Creational pattern scenarios */
///@{
DemoResult runAbstractFactory();
DemoResult runBuilder();
DemoResult runFactoryMethod();
DemoResult runPrototype();
DemoResult runSingleton();
///@}
}
namespace structural
{
/** @name Structural pattern scenarios */
///@{
DemoResult runAdapter();
DemoResult runBridge();
DemoResult runComposite();
DemoResult runDecorator();
DemoResult runFacade();
DemoResult runFlyweight();
DemoResult runProxy();
///@}
}
namespace behavioral
{
/** @name Behavioral pattern scenarios */
///@{
DemoResult runChainOfResponsibility();
DemoResult runCommand();
DemoResult runInterpreter();
DemoResult runIterator();
DemoResult runMediator();
DemoResult runMemento();
DemoResult runObserver();
DemoResult runState();
DemoResult runStrategy();
DemoResult runTemplateMethod();
DemoResult runVisitor();
///@}
}
/** @} */
} // namespace catalog
