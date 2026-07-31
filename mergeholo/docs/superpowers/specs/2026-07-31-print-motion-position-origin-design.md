# Print Motion Position And Origin Design

## Goal

Show live X/Y print positions and restore the V2 logical-origin workflow for
the two IMC60G print axes.

## Scope

- The print dialog shows X and Y positions in millimeters while a job runs.
- `Set Origin` makes the current stopped X/Y positions logical zero.
- `Return to Origin` moves X/Y back to that logical zero.
- The origin belongs only to the active EtherCAT connection. A disconnect,
  reconnect, or hardware homing operation restores the logical origin to
  `0,0`.

## Position Sampling

The position display uses the IMC60G planned-position register, matching the
current dialog display and V2's default position mode. X/Y pulse values are
converted with the active axis subdivision and resolution.

`PrintJobRunner::frameAdvanced` already runs inside the motion worker during
the print loop. The controller will use that signal to sample the two planned
positions at a fixed maximum rate of 10 Hz. Sampling remains on the worker
that owns IMC60G access, so it does not introduce concurrent SDK calls or a
second polling thread. Each successful sample is posted to the UI through the
existing `positionsChanged(double, double)` signal.

The existing 250 ms ready-state timer remains responsible for idle/manual
position updates. It does not poll during active printing.

## Logical Origin

Both actions are available only in `Ready` state.

`Set Origin` first stops both mapped axes and verifies that both have stopped.
It then applies the same IMC60G current-position reset and synchronization
sequence used after homing to X and Y, setting both logical positions to zero.
The controller immediately publishes `0.000 mm` for both axes.

`Return to Origin` uses the existing `returnToLogicalZero` and
`verifyLogicalZero` operations with the active X/Y axis profiles. Its timeout
is calculated per axis from the current planned distance and profile speed.
The operation succeeds only after both axes have stopped and the logical-zero
verification passes.

Neither command changes persistent configuration. A later connect-and-home
operation establishes a new hardware/logical zero, as in V2.

## UI And Errors

The manual-motion group gains `Set Origin` and `Return to Origin` command
buttons. Both are disabled while printing, paused, stopping, disconnected, or
faulted. Failures use the controller's existing error/status signals and move
the dialog into its existing fault workflow. The buttons do not bypass print
ownership or safety-stop checks.

## Tests

- Dialog tests verify button availability and one command dispatch per button.
- Controller tests verify frame-driven position publication while printing and
  the millimeter conversion.
- IMC60G motion tests verify that setting the origin stops both axes, resets
  both logical positions, and rejects unsafe states.
- Controller tests verify returning to origin invokes the current-profile
  zero-return and verification path, including the error path.
