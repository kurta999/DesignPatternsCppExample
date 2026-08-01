cmake_minimum_required(VERSION 3.20)
project(EmbeddedDesignPatterns LANGUAGES CXX)

add_executable(InverterStatePattern
    InverterStatePattern/src/InverterController.cpp
    InverterStatePattern/src/main.cpp)
target_include_directories(InverterStatePattern PRIVATE InverterStatePattern/include)

add_executable(EmbeddedPatternsCatalog
    EmbeddedPatternsCatalog/src/main.cpp
    EmbeddedPatternsCatalog/src/creational/AbstractFactory.cpp
    EmbeddedPatternsCatalog/src/creational/Builder.cpp
    EmbeddedPatternsCatalog/src/creational/FactoryMethod.cpp
    EmbeddedPatternsCatalog/src/creational/Prototype.cpp
    EmbeddedPatternsCatalog/src/creational/Singleton.cpp
    EmbeddedPatternsCatalog/src/structural/Adapter.cpp
    EmbeddedPatternsCatalog/src/structural/Bridge.cpp
    EmbeddedPatternsCatalog/src/structural/Composite.cpp
    EmbeddedPatternsCatalog/src/structural/Decorator.cpp
    EmbeddedPatternsCatalog/src/structural/Facade.cpp
    EmbeddedPatternsCatalog/src/structural/Flyweight.cpp
    EmbeddedPatternsCatalog/src/structural/Proxy.cpp
    EmbeddedPatternsCatalog/src/behavioral/ChainOfResponsibility.cpp
    EmbeddedPatternsCatalog/src/behavioral/Command.cpp
    EmbeddedPatternsCatalog/src/behavioral/Interpreter.cpp
    EmbeddedPatternsCatalog/src/behavioral/Iterator.cpp
    EmbeddedPatternsCatalog/src/behavioral/Mediator.cpp
    EmbeddedPatternsCatalog/src/behavioral/Memento.cpp
    EmbeddedPatternsCatalog/src/behavioral/Observer.cpp
    EmbeddedPatternsCatalog/src/behavioral/State.cpp
    EmbeddedPatternsCatalog/src/behavioral/Strategy.cpp
    EmbeddedPatternsCatalog/src/behavioral/TemplateMethod.cpp
    EmbeddedPatternsCatalog/src/behavioral/Visitor.cpp
    InverterStatePattern/src/InverterController.cpp)
target_include_directories(EmbeddedPatternsCatalog PRIVATE
    EmbeddedPatternsCatalog/include InverterStatePattern/include)

target_compile_features(InverterStatePattern PRIVATE cxx_std_17)
target_compile_features(EmbeddedPatternsCatalog PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(InverterStatePattern PRIVATE /W4 /permissive-)
    target_compile_options(EmbeddedPatternsCatalog PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(InverterStatePattern PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(EmbeddedPatternsCatalog PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_test(NAME inverter_state_scenarios COMMAND InverterStatePattern)
add_test(NAME all_gof_pattern_scenarios COMMAND EmbeddedPatternsCatalog)

