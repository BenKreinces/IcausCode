# Icarus Firmware Rewrite

## Mission

`IcarusSecondTry` is the clean-room replacement for the existing Icarus/HFTS
firmware projects. The end state is a single, maintainable STM32H743 firmware
project that reliably acquires sensors, controls actuators, and exchanges
telemetry and commands over Ethernet.

The old projects are references only. They are not architectural authorities.
Use them to recover verified board facts, pin assignments, protocol history, or
hardware workarounds, but do not copy their application structure or Ethernet
implementation without re-validating it.

## Scope

The target system includes:

- STM32H743 firmware on the Icarus custom board.
- Ethernet over RMII, including the KSZ8081 PHY.
- LwIP-based UDP communications.
- FreeRTOS task scheduling and synchronization.
- Pressure acquisition through AD7689 devices.
- Load-cell acquisition through the AD7124-8.
- Flowmeter capture using timer input capture and DMA.
- Motor control and telemetry through a VESC.
- Solenoid control through MCP23S17 expanders.
- Host-side tools and GUIs for control, telemetry, and diagnostics.

The system must be designed for hot-fire and pump-test operation. Safe startup,
safe fault handling, command timeouts, and observable diagnostics are required
features, not later enhancements.

## Project authority and repository policy

- All new firmware code goes under `IcarusSecondTry`.
- Do not add new functionality to the legacy `HFTS_*` projects.
- Keep `IcarusSecondTry` independently buildable and flashable at every stage.
- Preserve the old projects until the replacement passes the migration gates
  below. Treat their current state as frozen reference material.
- Do not delete or bulk-migrate legacy code as a shortcut.
- Keep hardware discoveries and deliberate design decisions documented in
  `IcarusSecondTry` (prefer a `docs/` directory for longer documents).
- Do not commit generated build output, debugger logs, binaries, or local IDE
  state unless the project explicitly requires that artifact.

The `.ioc` file and Cube-generated initialization are the source of truth for
pin and peripheral configuration. Application behavior belongs in handwritten
modules and must not depend on accidental generated-code details.

## Recommended source organization

Organize handwritten code into clear layers as the rewrite grows:

```text
IcarusSecondTry/
  Core/                 Cube-generated startup and HAL integration
  Drivers/              ST HAL/CMSIS and board-independent device drivers
  board/                Board pins, clocks, memory map, and hardware identity
  drivers/              Ethernet, ADC, flowmeter, VESC, and expander drivers
  services/             Protocol, telemetry, safety, health, and state services
  app/                  Tasks and product-level coordination
  tests/                Host-side protocol and pure-logic tests
  tools/                Host diagnostics and packet utilities
  docs/                 Bring-up procedures, packet format, and memory notes
```

The exact directories may be introduced incrementally, but dependencies should
flow downward:

```text
app -> services -> drivers -> board/HAL
```

Drivers must not depend on UDP, GUI code, product units, or application policy.
The network layer must not directly manipulate sensors or actuators.

## Development sequence

Implement and verify one vertical slice at a time. Do not enable every
peripheral and middleware component at once.

1. LED, reset behavior, clock, and debugger connection.
2. UART diagnostics, assertion handling, and fault reporting.
3. FreeRTOS scheduler with one trivial task and a known-good heartbeat.
4. GPIO and pin-map validation.
5. SPI bus validation and device identification.
6. Timer, interrupt, and DMA validation.
7. Ethernet MAC/RMII/KSZ8081 link diagnostics.
8. LwIP initialization and ping/ARP validation.
9. UDP echo with counters and packet diagnostics.
10. Versioned command and telemetry protocol.
11. Sensor drivers, one device class at a time.
12. Actuator drivers and safety state machine.
13. Host GUI integration.
14. Full-system hardware regression testing.

Each milestone must remain usable and should be tagged or otherwise recorded
before the next layer is added.

## FreeRTOS rules

FreeRTOS is the intended final execution model. Use it to isolate timing and
ownership, not to hide shared-state problems.

Expected task boundaries include:

- `NetworkTask`: owns network processing and network sockets/API calls.
- `CommandTask`: validates commands and forwards authorized actions.
- `SensorTask`: schedules sensor acquisition and publishes samples.
- `ActuatorTask`: owns motor and solenoid output state.
- `TelemetryTask`: builds periodic telemetry snapshots.
- `DiagnosticsTask`: health counters, logging, and service status.

These names are guidance, not a requirement to create all tasks immediately.

- Interrupt handlers must be short. Use task notifications, queues, or DMA
  completion signals to hand work to tasks.
- A peripheral has one clear owner. Do not access one SPI peripheral from
  multiple tasks without a documented ownership mechanism.
- Protect shared state with queues, mutexes, or immutable snapshots. Avoid
  `volatile` as a substitute for synchronization.
- Never block indefinitely on a queue, mutex, semaphore, or peripheral.
  Every wait involved in operation or safety behavior needs a timeout.
- Size stacks deliberately and monitor high-water marks.
- Monitor heap usage, queue saturation, missed deadlines, and task liveness.
- Keep time-critical acquisition independent from network traffic.
- Do not perform lengthy logging from an ISR or a high-rate acquisition path.

## Ethernet and LwIP rules

LwIP is required for the final Ethernet implementation, but network behavior
must be isolated and measurable.

- Start with a static IP for bring-up. Add DHCP only after the static path is
  reliable.
- Use one network owner task. Other tasks communicate with it through typed
  queues or message objects.
- Do not call LwIP APIs from arbitrary tasks or interrupt handlers.
- Keep command and telemetry traffic logically separate, preferably on separate
  UDP ports or message classes.
- Maintain explicit link state: initializing, down, negotiating, up, and fault.
- Record RX/TX packet counts, drops, malformed packets, socket errors, link
  transitions, and last-command/last-telemetry timestamps.
- Provide a diagnostic mode that can perform UDP echo without application
  sensors or actuators.
- Handle link loss and recovery without requiring a power cycle.
- Do not treat UDP delivery as reliable. Telemetry may be lossy; commands must
  be acknowledged and supervised.

### STM32H7 Ethernet memory and cache

The STM32H743 cache and DMA behavior must be handled centrally in the Ethernet
interface. Before declaring Ethernet reliable, verify all of the following:

- Ethernet descriptors are in DMA-accessible D2 SRAM.
- RX/TX buffers are in suitable DMA-accessible memory.
- MPU attributes for Ethernet memory are explicit and documented.
- Buffer alignment meets the DMA and cache-line requirements.
- TX buffers are cleaned before DMA reads them.
- RX descriptors and buffers are invalidated at the correct ownership points.
- Descriptor ownership transitions are correct on every success and error path.
- No application code performs ad hoc cache maintenance.
- The RMII reference clock, PHY reset, PHY address, MDIO/MDC, and link status
  are verified on actual hardware.

When Ethernet fails, diagnose physical link, PHY/MDIO, MAC/DMA, cache/MPU,
ARP/LwIP, and UDP/application protocol layers separately.

## Packet protocol rules

Never transmit a C struct by casting it to a byte buffer or relying on
`sizeof(struct)`. Compiler padding, alignment, enum sizes, and floating-point
representation must not define the wire format.

Every packet format must specify:

- Magic number.
- Protocol version.
- Message type.
- Header length and payload length.
- Sequence number.
- Timestamp or sample counter where applicable.
- Explicit byte order.
- Payload field widths and units.
- Optional CRC/checksum policy.

Serialization and parsing must be explicit and bounds-checked. Reject packets
with invalid magic, version, length, type, range, sequence, or CRC. Never allow
malformed network input to reach actuator code.

Telemetry should be periodic and sequence-numbered. It may be dropped or
reordered. Commands require an acknowledgment, command identifier, validation,
and a defined response for duplicate or stale commands. Heartbeats must be
separate from telemetry and must participate in connection supervision.

The host tools and firmware must share one documented protocol definition and
must have host-side tests for valid, truncated, oversized, reordered, repeated,
and deliberately corrupted packets.

## Safety and fault behavior

Define the safety state machine before integrating actuators. At minimum:

- Outputs start disabled and in a known safe state.
- A missing or stale command causes the motor to stop and solenoids to enter
  their defined safe state.
- Invalid commands are rejected without changing outputs.
- Sensor timeout, ADC communication failure, VESC failure, and task failure
  produce observable faults.
- Faults include a source, timestamp/counter, and clear/recovery policy.
- A hardware watchdog is enabled once the basic scheduler is stable.
- Watchdog servicing proves that critical tasks are alive; it must not be done
  unconditionally from a timer interrupt.
- Emergency shutdown behavior is independent of normal telemetry delivery.
- Startup and reconnect behavior cannot unexpectedly restore prior actuator
  commands.

Safety behavior must be tested with disconnected Ethernet, malformed packets,
stale packets, unplugged sensors, failed peripherals, and reset/reboot events.

## Driver rules

Each driver should expose initialization, health/status, operation, timeout,
and error reporting. Drivers should not silently retry forever or convert an
error into a plausible measurement.

- Use fixed-width integer types and explicit units.
- Validate device IDs and configuration readback where possible.
- Make SPI mode, word size, chip-select behavior, and timing explicit.
- Keep DMA buffers and ISR-shared data ownership documented.
- Return errors rather than hiding them in global flags.
- Keep conversion from raw code to engineering units outside the low-level bus
  driver unless the conversion is intrinsically part of the device driver.
- Include stuck-reading, saturation, timeout, and communication counters where
  they are meaningful.

## Generated code and editing rules

- Preserve CubeMX-generated regions and conventions.
- Put custom logic in `USER CODE` sections only when it must live in a
  generated file; prefer separate source files for substantial functionality.
- Regenerating CubeMX code must not erase application behavior.
- Review `.ioc` changes carefully, especially pin ownership, interrupt
  priorities, DMA streams, clocks, and middleware settings.
- Do not solve a configuration problem by manually editing generated files if
  the `.ioc` can express the intended configuration.
- After regeneration, inspect the diff and rebuild immediately.

## Testing and verification

Every change should be verified at the lowest practical level before hardware
testing:

- Build the affected STM32 project.
- Run host-side tests for packet and pure-logic changes.
- Check compiler warnings and linker memory usage.
- Test error and timeout paths, not only the nominal path.
- Record hardware test setup, firmware revision, network addresses, and result.

Ethernet acceptance requires, at minimum:

- Stable link detection across reset and cable reconnect.
- Successful ping/ARP after boot.
- Repeated UDP echo without drops or corruption during a sustained test.
- Correct handling of malformed and oversized packets.
- Correct packet sequence and length validation.
- Recovery after link loss.
- Command timeout causing the required safe output state.

Do not claim a peripheral works because it initializes without error. Verify
actual electrical activity, device identity/configuration, data readiness, and
meaningful measurements.

## Migration gates

Legacy projects may be removed only when all of these are true:

- `IcarusSecondTry` builds from a clean checkout without legacy source paths.
- Board bring-up, Ethernet, and packet protocol tests pass on hardware.
- Required sensor and actuator functions have replacement implementations.
- Host tools use the new protocol and no longer depend on legacy packet layouts.
- Safety and fault tests have been exercised and documented.
- Pin maps, network configuration, memory/cache rules, and packet format are
  documented in the new project.
- The old projects have been archived or tagged for historical reference.

Until then, preserve the old code and use Git history rather than destructive
file operations to recover prior work.

## Working style

Prefer small, reviewable changes with one stated purpose. Explain assumptions
about hardware or protocol behavior in code comments or documentation. When an
issue could be caused by multiple layers, add diagnostics to distinguish them
before changing several layers at once. Reliability and observability take
priority over feature breadth during bring-up.
