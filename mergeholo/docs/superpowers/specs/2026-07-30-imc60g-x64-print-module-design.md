# IMC60G x64 Print Module Replacement Design

## Status

Approved in conversation on 2026-07-30. Awaiting written-spec review before
implementation planning.

## Goal

Replace the existing `mergeholo` printing module with the production printing
behavior from `C:\wzp\Holographicface\按帧送图V2`, while keeping the application,
Qt, camera stack, and printing hardware integration uniformly x64.

The replacement must:

- call the IMC60G SDK through its native x64 library;
- preserve the current V2 frame delivery, scan timing, motion, homing, and
  SV660N exposure behavior;
- expose every V2 function that is useful and implementable on the new card;
- integrate those functions into the Qt printing UI with correct control
  enablement, validation, persistence, and status reporting;
- include and deploy all required headers, import libraries, runtime DLLs, and
  configuration files;
- fail closed when hardware, display synchronization, or exposure validation
  fails; and
- provide automated verification plus staged validation on the connected
  hardware.

## Confirmed Hardware Baseline

Only the IMC60G path is supported. The legacy DFJZH9030/6052 card, DLLs, API
branches, simulation paths, and unrelated test pages are out of scope.

The implementation strictly preserves the latest V2 hardware mapping:

- controller: IMC60G, card index 0;
- transport: EtherCAT;
- logical X axis: IMC axis 1;
- logical Y axis: IMC axis 0;
- exposure source axis: logical Y;
- exposure implementation: SV660N drive-internal position comparison;
- exposure output: the drive's `DO1+` / `DO1-`;
- SV660N SDO values, comparison point, attributes, width, and user-unit mode:
  taken from the latest V2 `MotionCardDefineConfig.h`;
- homing order and direction: preserve the latest V2 Y-then-X limit-homing
  behavior and configured directions;
- printing pulse constants, compensation values, and forward/reverse behavior:
  preserve the latest V2 definitions and active `start_new` path.

No software exposure or simulated-axis fallback may report a production
operation as successful.

## SDK and Bitness

The SDK at
`C:\wzp\Holographicface\按帧送图V2\IMC_Library_Release` contains:

- `IMC_Library.h`;
- `errorcode.h`;
- `x64\IMC_Library_x64.lib`; and
- `x64\IMC_Library_x64.dll`.

The x64 DLL is a PE32+ AMD64 binary. The SDK header is byte-for-byte identical
to the header used by V2. All 47 IMC API functions referenced by the latest V2
`MotionCardAdapter.cpp` are exported by the x64 DLL.

The required SDK artifacts will be placed under a stable repository-owned
`vendor/imc60g` layout rather than referenced through the V2 source directory.
The qmake project will link the x64 import library, and the build/deployment
script will copy the x64 runtime DLL beside `mergeholo.exe`. Build and startup
checks will reject missing files, a non-x64 DLL, or an unexpected loaded DLL
path.

## Architecture

### `Print9030Dialog`

The Qt dialog replaces the current simplified printing UI. It owns the
user-facing state and delegates all hardware work. It never calls IMC functions
directly.

It provides:

- memory-elemental and manual-folder image sources;
- folder selection and second-screen preview;
- V2 printing geometry, timing, scale, and pulse-compensation fields;
- editable axis profiles;
- hardware connection, homing, disconnection, and status;
- live position and axis-state display;
- manual axis movement and stop;
- print start, pause, resume, cancel, progress, and error log.

Controls are enabled from the state machine rather than independently. Parameters
that affect an active job are locked from preflight until cleanup finishes.

### `Imc60gMotionController`

This component is the only owner of the IMC SDK session. It ports the current
V2 IMC60G adapter behavior to Qt/C++ without MFC, old-card branches, or simulated
axes. It implements:

- card discovery and opening;
- EtherCAT scan, initialization, start, and status checks;
- configured software-emergency-state release;
- axis status clearing;
- axis discovery and logical-to-physical mapping;
- Servo On/Off;
- limit homing, backoff, position synchronization, and zeroing;
- motion profiles, absolute moves, jog/manual moves, stop, and position reads;
- SV660N SDO access required by the exposure controller; and
- deterministic shutdown.

The controller serializes all SDK calls on the printing worker thread. UI polling
is performed through queued controller requests so timer callbacks never call
the SDK concurrently with a print.

### `Sv660nExposureController`

This component owns the exposure configuration and arm/disarm sequence. It ports
the active V2 `SV660N_DRIVE_CMP` backend, including the exact SDO addresses,
values, axis selection, positive/negative crossing attributes, comparison
position, pulse width, and DO1 selection.

It verifies written values by reading them back when the SDK and drive permit.
Arming fails if configuration or verification fails. Every completion,
cancellation, error, disconnection, and close path attempts to disarm the
position comparison and force a safe output state before motion resources are
released.

### `V2D3DFramePresenter`

This component ports the latest V2 `D3DImageRenderer` and `SecondScr` behavior:

- select the intended non-primary display;
- create the presentation window and D3D11/DXGI resources;
- upload supported BGR/BGRA frames without intermediate image files;
- preserve the latest V2 refresh/presentation logic;
- wait on the selected output's VBlank;
- expose presentation and synchronization failures to the print engine; and
- release resources deterministically on shutdown.

The presentation operations are serialized and acknowledged synchronously so
the scan cannot advance before the required frame and VBlank operation
completes.

### `V2PrintEngine`

The engine is a direct behavioral port of the latest active V2 `start_new`
printing path. The formulas, pulse boundaries, row direction, image ordering,
VBlank timing, lead-pulse handling, extended constant-speed section, exposure
arming, and cleanup order are preserved. The port changes framework types and
module boundaries, not the production algorithm.

The engine consumes a validated immutable job snapshot. It never reads mutable
widgets or configuration files during a job.

### `PrintImageSource`

The existing memory-elemental and manual-folder inputs remain supported. Both
are converted into the same immutable frame sequence, with validated count,
dimensions, stride, channel order, and lifetime before hardware motion begins.

### `PrintHardwarePreflight`

Preflight runs before each print and verifies:

- x64 IMC runtime availability and expected load path;
- open card and healthy EtherCAT master;
- required real X/Y axes and exact mapping;
- Servo and homing state;
- valid SV660N exposure configuration;
- image count, dimensions, format, and grid compatibility;
- valid motion, timing, scale, and compensation values;
- available non-primary display;
- successful D3D preparation, initial frame presentation, and VBlank wait.

Any failed check prevents exposure arming and motion.

## Explicit Connection and Homing

Opening the printing dialog performs no motion and does not release software
emergency state or enable servos.

The user must click **连接并回零**. That command performs the V2 sequence:

1. discover and open the configured IMC60G;
2. initialize and validate EtherCAT;
3. release the configured software emergency state;
4. clear required axis states;
5. Servo On detected real axes;
6. home Y to its configured limit, back off, synchronize, and zero;
7. home X to its configured limit, back off, synchronize, and zero;
8. validate the final ready state.

Failure transitions to `Fault`, performs safe cleanup, and never enables print.
The user may retry only after the reported cause is resolved.

## UI Scope and Behavior

The UI keeps V2 functions that are relevant to the IMC60G production path:

- printing image source and folder selection;
- preview/second-screen display;
- move adjustment;
- row and column spacing;
- rows and columns;
- quiet-platform and exposure timing;
- output width and height scaling;
- extended constant-speed pulse count;
- forward/reverse lead compensation;
- axis subdivision, resolution, speed, acceleration, start velocity, stop
  velocity, maximum distance, and direction;
- controller/EtherCAT/Servo/homing state;
- live X/Y positions;
- manual X/Y movement and stop;
- connection and Y/X homing;
- print, pause, resume, cancel, progress, and status/error details.

Controls for additional physical axes are enabled only if the SDK discovers the
corresponding real axes and the mapping is configured. A missing Z/W/R axis is
shown as unavailable and is never simulated. Old-card controls, 6052 pages, and
unrelated experiments are removed.

All editable fields use typed Qt controls with explicit ranges and units.
Loading and saving use one versioned print configuration. Invalid or
inconsistent values are reported beside the affected field and block the
operation. Button labels, enabled states, and functions follow the controller
state:

```text
Disconnected -> Connecting/Homing -> Ready
Ready -> Printing <-> Paused
Printing/Paused -> Stopping -> Ready or Fault
Any safe state -> Disconnecting -> Disconnected
```

Closing during motion requests safe stop and waits for cleanup; it does not
destroy the SDK or D3D objects underneath an active worker.

## Print Data Flow

1. The dialog validates and saves user input.
2. It creates an immutable job containing frames, geometry, timing, axis
   profiles, and the confirmed hardware mapping.
3. Preflight validates hardware, exposure configuration, frames, display, and
   VBlank.
4. The engine executes the V2 homed-origin preparation and serpentine print.
5. Each frame selection, present, VBlank wait, motion transition, and exposure
   transition is logged with row/column/direction and hardware positions.
6. Pause stops at a V2-compatible safe boundary and leaves exposure disarmed.
   Resume revalidates the state before continuing.
7. Completion or cancellation runs the common safe cleanup path and reports the
   final state to the dialog.

## Error Handling and Safety

There is no silent fallback for SDK load, board access, EtherCAT, missing axes,
Servo, homing, exposure, display, frame decoding, D3D, or VBlank failures.

The common cleanup order is:

1. disarm SV660N position comparison and drive DO1 to the configured safe state;
2. request stop for active axes;
3. wait for confirmed stop within bounded timeouts;
4. return X/Y to the homed origin only when controller state and safety
   conditions permit;
5. release presentation resources;
6. on disconnect/close, Servo Off, stop EtherCAT, close the card, and unload the
   SDK session.

Cleanup failures are appended to the original error. A job is not reported as
successful unless exposure is safe, axes are stopped, and the required
post-print position condition is met.

Every IMC and SV660N call records the function, logical and physical axis,
parameters, return code, decoded error text, and state transition. Logs exclude
unbounded frame data.

## Configuration

One versioned configuration replaces split or duplicated printing settings. It
contains:

- immutable production hardware profile: card index, X/Y mapping, exposure axis,
  SV660N comparison/DO1 values, homing order, directions, and safety timeouts;
- editable print profile: geometry, grid, timing, scale, and pulse
  compensation;
- editable motion profiles within safe validated limits;
- image-source preferences and display selection.

Production-critical hardware mapping is displayed in the UI but cannot be
accidentally edited from ordinary print controls. Changing that profile requires
an explicit service/configuration action and triggers a full reconnect and
rehoming.

## Verification Strategy

### Automated tests without hardware

- verify SDK headers, import library, runtime DLL, PE architecture, required API
  exports, and deployment destination;
- compare V2 golden input cases against the ported timing, pulse, exposure,
  row-direction, and frame-order calculations;
- use recording fakes to verify the complete IMC/SV660N call order for connect,
  home, manual motion, forward and reverse rows, pause/resume, cancellation,
  failures, cleanup, and disconnect;
- verify no motion begins when any preflight check fails;
- verify exposure is disarmed on every terminal path;
- test memory and folder frame validation and channel/stride conversion;
- run Qt UI tests for every button, field, validation error, enabled state,
  persistence path, progress update, and close-during-print path;
- perform a clean x64 qmake build and dependency staging test.

### Staged connected-hardware validation

The connected hardware is available and approved for validation. Testing
progresses only after each stage passes:

1. SDK load, card discovery, EtherCAT scan/status, and axis mapping;
2. explicit connection, Servo On, Y/X limit homing, backoff, and zero;
3. low-speed, small-distance X/Y moves in both directions, stop, and position
   verification;
4. second-screen selection, frame order, refresh behavior, and VBlank logs;
5. SV660N SDO read/write verification and DO1 pulse measurement using an
   oscilloscope or safe load;
6. one-row low-risk print;
7. small multi-row serpentine print including reverse direction;
8. pause, resume, cancel, error injection, safe exposure shutdown, and return
   behavior;
9. final end-to-end print from both memory-elemental and folder sources.

Observed positions, pulse timing, DO1 waveform, image order, log events, and
cleanup results are retained as acceptance evidence.

## Acceptance Criteria

- `mergeholo.exe`, Qt, all printing code, and the IMC runtime are x64.
- No DFJZH or x86 printing dependency is loaded or required.
- All required V2 IMC60G production functions are present in the Qt UI and
  operate according to their labels and fields.
- Entering the dialog causes no hardware motion.
- **连接并回零** performs the confirmed V2 IMC/EtherCAT/Servo/Y-X homing sequence.
- The exact confirmed X/Y mapping and SV660N DO1 exposure backend are used.
- The V2 print formulas, frame order, VBlank behavior, and exposure timing pass
  golden tests and connected-hardware checks.
- Invalid inputs and unavailable hardware block motion with actionable errors.
- All cancellation, error, and close paths leave exposure safe and axes stopped.
- Clean build and deployment include every required SDK and runtime artifact.
- The staged hardware acceptance sequence completes with recorded evidence.

## Non-goals

- Legacy DFJZH9030/6052 support.
- x86 application or bridge-process support.
- Simulated production axes or software-simulated production exposure.
- Retaining MFC UI implementation details.
- Unrelated V2 test dialogs and experiments.
- Redesigning the calibrated V2 production scan algorithm.
