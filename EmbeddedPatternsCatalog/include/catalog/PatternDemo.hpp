#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace catalog
{
struct DemoResult
{
    std::string_view category;
    std::string_view pattern;
    std::string outcome;
};
using DemoFunction = DemoResult (*)();

inline void require(const bool condition, const std::string_view message)
{
    if (!condition) throw std::logic_error(std::string{message});
}

namespace creational
{
DemoResult runAbstractFactory();
DemoResult runBuilder();
DemoResult runFactoryMethod();
DemoResult runPrototype();
DemoResult runSingleton();
}
namespace structural
{
DemoResult runAdapter();
DemoResult runBridge();
DemoResult runComposite();
DemoResult runDecorator();
DemoResult runFacade();
DemoResult runFlyweight();
DemoResult runProxy();
}
namespace behavioral
{
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
}
} // namespace catalog

