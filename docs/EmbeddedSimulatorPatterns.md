# Design patterns inside EmbeddedSimulator {#embedded_simulator_patterns}

This page documents the patterns that participate in the connected desktop
simulator. It is deliberately implementation-oriented: each row identifies the
participants, their source locations, and the reason the pattern exists in this
application.

## Runtime collaboration

```text
MainFrame
  |  queues Start/Pause/Reset/FaultReset commands
  v
SimulationCommandQueue ---> ISimulationControl
                                  |
                                  v
                          SimulationEngine
                         /    |      |     \
               DeviceFactory |      |      ILogger
                /             |      |          \
    IMeasurementDriver  ICoolingStrategy         ILogSink(s)
                \             /      |
                 PlantState --------> InverterController
                                          |
                                  SimulationSnapshot
                                          |
                                   ISimulationObserver
                                          |
                                       MainFrame
```

At every GUI timer tick, `MainFrame` first dispatches queued commands, then
updates `SimulationControls`, and finally calls `SimulationEngine::step()`. The
engine updates the plant, samples it through the selected measurement driver,
runs the cooling policy and inverter State machine, logs important changes, and
publishes one immutable snapshot to the GUI.

## Pattern map

| Pattern | Intent in this simulator | Participants | Implementation location |
|---|---|---|---|
| **Abstract Factory** | Keep each MCU board's peripheral names, ADC resolution, noise, and sensor hookup mutually compatible. | Internal `IBoardDriverFactory`; `Stm32H7DriverFactory`; `NxpS32K3DriverFactory` | `EmbeddedSimulator/src/DeviceFactory.cpp` |
| **Factory** | Centralize runtime selection without spreading `switch` statements through the GUI or engine. | `DeviceFactory::createMeasurementDriver()`; `DeviceFactory::createCoolingStrategy()` | `EmbeddedSimulator/include/simulator/DeviceFactory.hpp`, `EmbeddedSimulator/src/DeviceFactory.cpp` |
| **Adapter** | Convert board/frontend-specific sampling characteristics and raw plant values into the stable physical-unit structure consumed by the controller. | `IMeasurementDriver`; internal `SimulatedMeasurementDriver`; `PlantState`; `SensorReadings` | `EmbeddedSimulator/include/simulator/DeviceFactory.hpp`, `EmbeddedSimulator/src/DeviceFactory.cpp` |
| **State** | Put startup, precharge, contactor overlap, drive, trip, and guarded-reset rules in state-specific objects rather than a monolithic conditional. | `InverterController` context; internal `State`; `StandbyState`; `PrechargingState`; `MainContactorClosingState`; `ReadyState`; `DrivingState`; `FaultLatchedState` | `InverterStatePattern/include/firmware/InverterController.hpp`, `InverterStatePattern/src/InverterController.cpp` |
| **Strategy** | Allow quiet and performance fan policies to vary independently of simulation sequencing. | `ICoolingStrategy`; internal `QuietCoolingStrategy`; `PerformanceCoolingStrategy` | `EmbeddedSimulator/include/simulator/DeviceFactory.hpp`, `EmbeddedSimulator/src/DeviceFactory.cpp` |
| **Observer** | Publish measurements and controller results without introducing wxWidgets into the simulation core. | `ISimulationObserver`; `SimulationEngine::attachObserver()`; `MainFrame::onSimulationSnapshot()` | `EmbeddedSimulator/include/simulator/SimulationEngine.hpp`, `EmbeddedSimulator/include/gui/MainFrame.hpp`, `EmbeddedSimulator/src/SimulationEngine.cpp` |
| **Command** | Represent operator actions as queueable requests and keep wx event handlers independent of the command receiver. | `ISimulationCommand`; four concrete command classes; `SimulationCommandQueue`; `ISimulationControl` receiver interface | `EmbeddedSimulator/include/simulator/SimulationCommands.hpp`, `EmbeddedSimulator/src/gui/MainFrame.cpp` |
| **Facade** | Expose one small application-facing API for configuration, controls, stepping, state inspection, and lifecycle operations. | `SimulationEngine` | `EmbeddedSimulator/include/simulator/SimulationEngine.hpp` |
| **Mediator** | Coordinate the plant, driver, cooling policy, inverter controller, observers, and logger without those collaborators knowing about one another. | `SimulationEngine::Impl` | `EmbeddedSimulator/src/SimulationEngine.cpp` |
| **Observer-style logging sinks** | Fan one log record out to persistent, in-memory, or GUI callback destinations. | `Logger`; `ILogSink`; `FileLogSink`; `MemoryLogSink`; `CallbackLogSink` | `EmbeddedSimulator/include/simulator/Logging.hpp`, `EmbeddedSimulator/src/Logging.cpp` |

## Supporting design techniques

These are not additional Gang of Four patterns, but they are important to the
quality of the implementation:

| Technique | Where it is used | Why it matters |
|---|---|---|
| **Dependency injection** | `SimulationEngine(ILogger&)` and logger sink registration | Tests inject an in-memory sink while the GUI composes file and callback sinks. The engine never constructs a global logger. |
| **Dependency inversion** | `ILogger`, `ILogSink`, `IMeasurementDriver`, `ICoolingStrategy`, `ISimulationObserver`, `ISimulationControl` | Policy code depends on focused abstractions rather than wxWidgets or concrete devices. |
| **PImpl** | `SimulationEngine::Impl` and `FileLogSink::Impl` | Hides changing implementation details and keeps public headers smaller. PImpl is a C++ idiom, not a GoF pattern. |
| **Composition root** | `MainFrame` constructor | The GUI owns and connects the logger, sinks, engine, command queue, and observer relationship in one visible place. |

## End-to-end examples

### Changing the board and sensor frontend

1. `MainFrame::applySelectedConfiguration()` constructs a
   `SimulationConfiguration` from the selected controls.
2. `SimulationEngine::applyConfiguration()` asks `DeviceFactory` for compatible
   objects.
3. The selected concrete board factory creates a measurement driver with the
   correct peripheral description, resolution, and deterministic measurement
   error.
4. The engine sees only `IMeasurementDriver`; neither the State machine nor GUI
   contains MCU-specific branches.

This path connects **Factory**, **Abstract Factory**, **Adapter**, **Facade**, and
**dependency inversion**.

### Starting and driving the inverter

1. A wx button handler enqueues `StartCommand`.
2. `SimulationCommandQueue::dispatchAll()` invokes the `ISimulationControl`
   receiver.
3. Each periodic engine step feeds sampled inputs into `InverterController`.
4. The State machine advances through precharge and contactor states.
5. The engine applies the selected cooling Strategy and updates the plant.
6. A `SimulationSnapshot` is sent to the `MainFrame` Observer and state changes
   are sent to every configured log sink.

This path connects **Command**, **Facade**, **Mediator**, **State**, **Strategy**,
and **Observer**.

### Handling an overcurrent fault

1. The GUI changes only `SimulationControls::forcePhaseOvercurrent`.
2. The plant produces a high phase current and the measurement Adapter samples it.
3. The State context applies cross-cutting protection and enters
   `FaultLatchedState`, which immediately commands safe outputs.
4. The Mediator records the transition and publishes the new snapshot.
5. The GUI Observer updates the fault label, power-path status, graph, and log.
6. A queued `FaultResetCommand` is accepted only after ignition, current, and
   temperature satisfy the controller's reset contract.

## Ownership and lifetime

- `MainFrame` owns `Logger`, `SimulationEngine`, and `SimulationCommandQueue`.
- `Logger` shares ownership of its sinks through `std::shared_ptr` because GUI,
  file, and tests may retain a sink handle.
- `SimulationEngine::Impl` exclusively owns its measurement driver and cooling
  policy through `std::unique_ptr`.
- The engine stores non-owning observer pointers. An observer must outlive the
  engine or otherwise detach before destruction. In this application,
  `MainFrame` is both owner of the engine and its observer, so that lifetime is
  deterministic.
- Static State objects contain no mutable state; per-run state remains inside
  `InverterController`, avoiding allocation in its periodic control path.

## Patterns intentionally not forced into the simulator

The catalog project demonstrates all 23 GoF patterns, but the connected
simulator uses only patterns that solve an actual collaboration or variability
problem. For example, there is no useful need for a Visitor over the plant model
or a Singleton logger. Adding them solely to increase a pattern count would make
the production-shaped example harder to maintain.

