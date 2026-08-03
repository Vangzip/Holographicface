# Negative Reverse Lead Pulse Design

## Goal

Allow the IMC60G 9030 print parameter `lead_pulse` (UI label: `反向提前`) to be negative while preserving the strict current V2 timing formula and all existing hardware-safety checks.

## Confirmed semantics

- `add_temp_pulse` (`延长匀速段`) remains non-negative.
- `lead_pulse` accepts the UI's existing signed int32 range: `-2147483647` through `2147483647` pulse.
- The value is saved to and loaded from `config/print_9030.ini` without clamping or changing its sign.
- The active V2 formula remains:

  ```text
  reverseDelayPulse = addTempPulse
                    - reverseFixedPulse
                    - forwardDelayPulse
                    - leadPulse
  ```

  Therefore a negative `leadPulse` increases the computed reverse display delay by its absolute value. For example, `leadPulse=-5000` contributes `+5000 pulse` to `reverseDelayPulse`.
- This change does not alter row motion, exposure comparison positions, axis mapping, homing, EtherCAT calls, or SV660N DO1 behavior.

## Implementation boundaries

1. Split the current combined validation so only negative `addTempPulse` is rejected. Do not reject `leadPulse` solely because it is negative.
2. Preserve the existing checked subtraction used to calculate `reverseDelayPulse`. Arithmetic overflow must still reject the plan before motion or exposure.
3. Preserve the existing conversion of non-positive computed reverse delay to zero VBlank hold frames.
4. Keep the existing signed range of `leadPulseSpin`; no UI layout change is required.
5. Update the operator documentation to state that `lead_pulse` is signed and explain its effect in the formula.

## Tests

- Add a V2 timing regression using `leadPulse=-5000` and assert that plan generation succeeds and that the reverse-row delay reflects subtraction of the negative value.
- Add a validation regression proving negative `addTempPulse` is still rejected.
- Update the print-dialog regression to enter, start with, save, and reload a negative `leadPulse` unchanged.
- Run the complete printing test suite, x64 production build, SDK/runtime checks, configuration deployment checks, and diagnostics-only startup probe.

## Safety

All automated verification remains hardware-silent. Opening or testing the dialog must not open Card0, initialize EtherCAT, enable servos, home axes, move axes, or toggle DO1. Real hardware testing still requires a fresh现场 safety confirmation.
