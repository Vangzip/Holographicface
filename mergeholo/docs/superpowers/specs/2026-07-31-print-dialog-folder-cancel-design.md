# Print Dialog Folder Loading and Cancellation Design

## Scope

Improve the IMC60G print dialog in four connected areas:

- align the print-parameter editors;
- load manually selected image folders without blocking the GUI;
- infer grid dimensions from ordered image names when possible;
- reset a cancelled print for another run only after verified hardware cleanup.

The existing EtherCAT, Servo On, homing, and print hardware mapping are out of
scope except where cancellation cleanup needs a correct motion timeout.

## Parameter Layout

`mainGrid` will use two symmetric label/editor pairs per row while preserving
every existing widget object name:

| Row | Left pair | Right pair |
| --- | --- | --- |
| 0 | row spacing | column spacing |
| 1 | rows | columns |
| 2 | output width scale | output height scale |
| 3 | constant-speed extension | reverse lead |

This is a `.ui`-only layout change. Configuration serialization and test widget
lookups remain unchanged.

## Folder Source Loading

`Print9030Dialog` will own a `QFutureWatcher` for folder loading. Selecting or
typing a folder path starts a `QtConcurrent::run` worker that enumerates and
decodes the images. The GUI thread only starts the work and applies its final
result.

While the worker runs, the dialog shows a loading source summary and disables
source editing, preview, and Start. A completion result replaces the folder
image set and updates the source summary atomically. Failed loads leave the
folder source invalid and show the supplied diagnostic. A request identifier
ensures an obsolete completion cannot replace a newer selection.

The image-source module will return optional grid metadata with the loaded
images. It recognizes exactly six digit basenames in the form `RRRCCC`, where
both three-digit parts are one-based row and column identifiers. Automatic grid
metadata is accepted only when all names follow the format and they form one
complete rectangle without duplicate cells. Frames are then ordered row-major.
For example, `001001.jpg` through `100100.jpg` produces 100 rows and 100
columns. Any other naming arrangement still loads in the existing lexical order
but does not overwrite manually configured row and column values; the dialog
reports that dimensions were not inferred.

## Cancellation Lifecycle

The runner remains the authority for safety. After a cancellation request has
finished cleanup it emits progress zero, including when cleanup reports an
error. The controller clears the transient cancellation diagnostic only when
the runner has reached `Ready`.

A verified cancel performs exposure disarm, stops both axes, verifies both are
stopped, returns both to logical zero, and verifies zero. It then enters
`Ready`, so Start is enabled for a valid source. A cleanup failure remains
`Fault`; Start stays disabled and the user must reconnect and home after
correcting the hardware condition. Resetting the progress bar must not weaken
this safety gate.

The existing cleanup timeout is incorrect because it is derived solely from
the planned Y-axis travel and reused for X-axis zero return. The replacement
calculates a timeout for each return-to-zero move from that axis's current
planned position, pulse-per-millimeter configuration, speed, acceleration,
and a bounded settling allowance. This prevents a slow but valid X-axis return
from timing out based on a fast Y-axis profile. A true no-stop or alarm still
reports Fault.

## Tests

- Extend image-source tests for complete `RRRCCC` grids, row-major ordering,
  and non-inferable names.
- Extend dialog tests to wait for asynchronous completion, verify inferred
  rows and columns, verify source controls remain disabled while loading, and
  verify the aligned layout geometry.
- Extend print-engine tests to prove cancellation publishes zero progress and
  a safely cleaned job reaches `Ready`.
- Extend IMC motion tests with a slow X-axis return whose required time exceeds
  the Y-based legacy timeout but is within the axis-specific budget.
- Run the focused print dialog, print engine, image-source, and IMC tests,
  followed by the Release build.
