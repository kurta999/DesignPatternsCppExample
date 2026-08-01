cmake_minimum_required(VERSION 3.20)
project(EmbeddedDesignPatterns LANGUAGES CXX)

option(EMBEDDED_PATTERNS_BUILD_GUI "Build the wxWidgets simulation application" ON)
option(EMBEDDED_PATTERNS_BUILD_DOCS "Add the Doxygen HTML documentation target" OFF)

if(EMBEDDED_PATTERNS_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    add_custom_target(documentation
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/out/docs"
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_SOURCE_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating complete Embedded Design Patterns API documentation"
        VERBATIM)
endif()

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

set(EMBEDDED_SIMULATOR_CORE_SOURCES
    EmbeddedSimulator/src/SimulationTypes.cpp
    EmbeddedSimulator/src/Logging.cpp
    EmbeddedSimulator/src/DeviceFactory.cpp
    EmbeddedSimulator/src/SimulationEngine.cpp
    InverterStatePattern/src/InverterController.cpp)

add_library(EmbeddedSimulatorCore STATIC ${EMBEDDED_SIMULATOR_CORE_SOURCES})
target_include_directories(EmbeddedSimulatorCore PUBLIC
    EmbeddedSimulator/include
    InverterStatePattern/include)
target_compile_features(EmbeddedSimulatorCore PUBLIC cxx_std_17)

add_executable(EmbeddedSimulatorTests
    EmbeddedSimulator/tests/SimulationEngineTests.cpp)
target_link_libraries(EmbeddedSimulatorTests PRIVATE EmbeddedSimulatorCore)
target_compile_features(EmbeddedSimulatorTests PRIVATE cxx_std_17)

if(EMBEDDED_PATTERNS_BUILD_GUI)
    find_package(wxWidgets CONFIG REQUIRED)
    add_executable(EmbeddedSimulator WIN32
        EmbeddedSimulator/src/gui/SimulatorApp.cpp
        EmbeddedSimulator/src/gui/MainFrame.cpp)
    target_link_libraries(EmbeddedSimulator PRIVATE
        EmbeddedSimulatorCore
        wx::core
        wx::base)
    target_compile_features(EmbeddedSimulator PRIVATE cxx_std_17)
endif()

target_compile_features(InverterStatePattern PRIVATE cxx_std_17)
target_compile_features(EmbeddedPatternsCatalog PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(InverterStatePattern PRIVATE /W4 /permissive-)
    target_compile_options(EmbeddedPatternsCatalog PRIVATE /W4 /WX /permissive-)
    target_compile_options(EmbeddedSimulatorCore PRIVATE /W4 /WX /permissive-)
    target_compile_options(EmbeddedSimulatorTests PRIVATE /W4 /WX /permissive-)
    if(TARGET EmbeddedSimulator)
        target_compile_options(EmbeddedSimulator PRIVATE /W4 /WX /permissive- /utf-8)
    endif()
else()
    target_compile_options(InverterStatePattern PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(EmbeddedPatternsCatalog PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(EmbeddedSimulatorCore PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(EmbeddedSimulatorTests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    if(TARGET EmbeddedSimulator)
        target_compile_options(EmbeddedSimulator PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endif()

enable_testing()
add_test(NAME inverter_state_scenarios COMMAND InverterStatePattern)
add_test(NAME all_gof_pattern_scenarios COMMAND EmbeddedPatternsCatalog)
add_test(NAME connected_simulator_engine_scenarios COMMAND EmbeddedSimulatorTests)
