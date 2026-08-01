/**
 * @file
 * @brief Console runner for all 23 executable Gang of Four examples.
 */

#include "catalog/PatternDemo.hpp"

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    using namespace catalog;
    constexpr std::array<DemoFunction, 23> demos{
        creational::runAbstractFactory, creational::runBuilder,
        creational::runFactoryMethod, creational::runPrototype,
        creational::runSingleton, structural::runAdapter, structural::runBridge,
        structural::runComposite, structural::runDecorator, structural::runFacade,
        structural::runFlyweight, structural::runProxy,
        behavioral::runChainOfResponsibility, behavioral::runCommand,
        behavioral::runInterpreter, behavioral::runIterator, behavioral::runMediator,
        behavioral::runMemento, behavioral::runObserver, behavioral::runState,
        behavioral::runStrategy, behavioral::runTemplateMethod, behavioral::runVisitor};

    try
    {
        for (const auto demo : demos)
        {
            const auto result = demo();
            std::cout << '[' << result.category << "] " << result.pattern
                      << "\n  " << result.outcome << "\n";
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Pattern demo failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "\nAll 23 GoF embedded pattern examples passed.\n";
    return EXIT_SUCCESS;
}
