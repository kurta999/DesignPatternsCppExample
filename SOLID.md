# SOLID Review of the Pattern Catalog

Design patterns do not automatically make code SOLID. A pattern describes a
collaboration; SOLID evaluates responsibilities, extension points, substitution,
interface size, and dependency direction. This catalog applies the principles as
engineering constraints rather than claiming that every pattern demonstrates all
five principles equally.

## Principles used throughout

- **Single Responsibility (SRP):** policy, hardware I/O, data storage, formatting,
  and orchestration are kept in separate classes when they have different reasons
  to change.
- **Open/Closed (OCP):** new drivers, strategies, handlers, commands, expressions,
  states, and outputs are normally added as new implementations rather than by
  editing stable clients.
- **Liskov Substitution (LSP):** derived implementations preserve their interface
  contracts, base destructors are virtual, and clients operate through the base
  abstraction without concrete-type checks.
- **Interface Segregation (ISP):** hardware boundaries expose small capability
  interfaces such as `IWatchdogHardware`, `IImageStorage`,
  `ICanCommandTransport`, and `IChargeContactor`.
- **Dependency Inversion (DIP):** supervisory policy depends on interfaces or
  plain input/output values. Concrete simulated drivers are created only in the
  composition code inside each `run...()` demo.

## Pattern-by-pattern mapping

| Pattern | Main SOLID contribution | Evidence in the example |
|---|---|---|
| Abstract Factory | OCP, ISP, DIP | Diagnostics consumes driver interfaces; each BSP creates a compatible family |
| Builder | SRP | CAN-FD construction, byte order, and validation live in the builder |
| Factory Method | OCP, LSP, DIP | The stable monitor algorithm accepts probe selection from subclasses |
| Prototype | OCP, LSP | Profiles clone through one contract and clones remain independently substitutable |
| Singleton | SRP, ISP, DIP | Heartbeat policy is separate from `IWatchdogHardware` I/O |
| Adapter | SRP, DIP | Legacy conversion is isolated behind `IVoltageSensor` |
| Bridge | OCP, ISP, DIP | Alarm meaning and output transport vary independently |
| Composite | OCP, LSP | Leaves and branches honor the same power-node contract |
| Decorator | OCP, LSP, DIP | Sequence and CRC wrappers remain valid frame senders |
| Facade | SRP, ISP, DIP | Update sequencing depends on safety, storage, and boot capability interfaces |
| Flyweight | SRP | The repository owns shared curve creation; channels own only extrinsic state |
| Proxy | LSP, ISP, DIP | The remote drive proxy implements `IMotorDrive` and uses `ICanCommandTransport` |
| Chain of Responsibility | OCP, LSP | New protection handlers extend the chain without changing existing handlers |
| Command | SRP, OCP, DIP | Queue/history depends on `ICommand`; commands isolate actuator operations |
| Interpreter | OCP, LSP | New terminal or composite rules implement `IExpression` |
| Iterator | SRP, ISP | Ring storage and traversal mechanics are separate; clients see iterator operations |
| Mediator | SRP, ISP, DIP | Interaction policy is centralized and contactor I/O uses a narrow interface |
| Memento | SRP | Calibration owns state; its memento owns an opaque snapshot only |
| Observer | OCP, ISP, DIP | Publisher knows only the one-method observer interface |
| State | SRP, OCP, LSP | Each operating mode owns its legal behavior; the context owns transitions and common protection |
| Strategy | OCP, LSP, DIP | Cooling policy is injected and selectable through `IFanStrategy` |
| Template Method | OCP, LSP | Derived sensor tests customize defined steps without changing the safe sequence |
| Visitor | SRP, OCP for operations | Power and safety operations are separate visitors over a stable hardware family |

## Honest tradeoffs

SOLID principles are not independent checkboxes, and some GoF patterns deliberately
choose one extension axis over another:

- **Singleton** still introduces global access and first-use initialization. The
  example minimizes the damage with an injected hardware interface, but explicit
  ownership is preferable unless the hardware truly has one process-wide owner.
- **Visitor** is open for new operations but not for new element types. Adding a
  new hardware element requires updating the visitor interface and all visitors.
- **Facade** should remain thin. Business rules belong in domain services; the
  facade should only enforce the safe subsystem sequence.
- **LSP** requires semantic contracts, not merely matching method signatures. For
  example, every `IMotorDrive` must reject unsafe commands consistently, and every
  `IPowerNode::shutdown()` must leave the represented subtree de-energized.
- Embedded targets may replace heap-backed containers and runtime polymorphism
  with fixed-capacity storage or compile-time policies. That changes mechanics,
  not the dependency direction or responsibility boundaries.

## SOLID in the connected GUI

The wxWidgets application keeps the framework at the outer boundary:

- `SimulationEngine` depends on `ILogger`, measurement-driver, cooling-strategy,
  observer, and plain input/output abstractions rather than on GUI controls.
- `MainFrame` translates user actions into Commands and renders Observer
  snapshots; it does not implement plant or protection rules.
- `DeviceFactory` is the composition root for concrete board drivers.
- The existing `InverterController` is reused unchanged, demonstrating that the
  domain State machine is independent of both wxWidgets and vcpkg.
- `EmbeddedSimulatorTests` links only the core and checks every selectable device
  combination without creating a native window.
