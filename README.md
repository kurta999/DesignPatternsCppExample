# Embedded Design Patterns in C++

This Visual Studio solution contains practical C++17 examples of **all 23 Gang
of Four design patterns**. The scenarios come from embedded systems, vehicle
electronics, motor controls, battery systems, industrial machines, diagnostics,
and firmware update flows.

## Solution projects

- **EmbeddedPatternsCatalog** runs all 23 compact examples. Each pattern has its
  own `.cpp` file and a Creational, Structural, or Behavioral Visual Studio filter.
- **InverterStatePattern** is a larger standalone example modeling DC-link
  precharge, contactor overlap, drive enable, fault latching, and guarded reset
  without heap allocation.

Set `EmbeddedPatternsCatalog` as the startup project for the complete tour.

## Pattern index

### Creational

| Pattern | Embedded scenario |
|---|---|
| Abstract Factory | Select matching ADC and CAN drivers for STM32 or NXP BSPs |
| Builder | Build and validate a byte-ordered CAN-FD BMS frame |
| Factory Method | Select PT100 or thermocouple probes in monitor subclasses |
| Prototype | Clone qualified pressure-channel calibration |
| Singleton | Give a hardware watchdog exactly one supervisor |

### Structural

| Pattern | Embedded scenario |
|---|---|
| Adapter | Convert a legacy ADC-count API into physical volts |
| Bridge | Send one thermal alarm over CAN or a local buzzer |
| Composite | Meter and recursively shut down a power tree |
| Decorator | Add sequence and CRC layers to a CAN sender |
| Facade | Coordinate interlock, flash, and boot-slot update steps |
| Flyweight | Share immutable thermistor calibration across channels |
| Proxy | Validate and cache commands to a remote CAN pump |

### Behavioral

| Pattern | Embedded scenario |
|---|---|
| Chain of Responsibility | Prioritize emergency, thermal, and pressure faults |
| Command | Execute and undo a coolant drain-valve operation |
| Interpreter | Evaluate a composable machine-start interlock rule |
| Iterator | Traverse a wrapped fault ring buffer chronologically |
| Mediator | Coordinate charger pilot, battery temperature, and contactor |
| Memento | Roll back an invalid motor calibration |
| Observer | Publish cell voltage to protection, dashboard, and log |
| State | Supervise inverter precharge, ready, drive, and fault modes |
| Strategy | Select quiet or maximum battery fan control |
| Template Method | Reuse a safe sensor self-test sequence |
| Visitor | Apply power-budget and safety-audit operations to hardware |

## Build with Visual Studio

Open `EmbeddedDesignPatterns.sln` in Visual Studio 2022 and build the solution.
Both `Debug`/`Release` and `Win32`/`x64` configurations are included.

```powershell
msbuild .\EmbeddedDesignPatterns.sln /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\EmbeddedPatternsCatalog.exe
```

Expected final line:

```text
All 23 GoF embedded pattern examples passed.
```

## Optional CMake build

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Embedded engineering notes

Patterns are vocabulary, not targets. Use one when it removes real coupling or
conditional complexity. The catalog uses standard containers, smart pointers,
exceptions, and virtual calls to make each pattern obvious on a PC. On a
constrained target, consider fixed-capacity containers, statically allocated
object graphs, error-return types, and compile-time strategies.

The standalone State controller demonstrates a stricter style: static state
objects, plain input/output structs, supplied monotonic time, immediate safe
outputs on fault, and no OS or console dependency.

Treat Singleton with suspicion. A hardware watchdog can have one legitimate
owner; ordinary services usually benefit from explicit construction and injected
references instead of hidden global state.

