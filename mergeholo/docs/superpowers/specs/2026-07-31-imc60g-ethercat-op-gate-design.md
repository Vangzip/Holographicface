# IMC60G EtherCAT OP Gate Design

## Goal

Keep the existing automatic IMC60G connection flow. After the SDK scans
EtherCAT, require the master to report OP and at least the configured X/Y
axes to be discovered before issuing any Servo On command.

## Connection Flow

`connectAndHome()` will retain the current sequence:

1. Discover and open Card0.
2. Call `IMC_ScanCardEcat(Card0, 40)`; this is the automatic SDK operation
   that waits for OP, scans the bus, and establishes communication.
3. Read `IMC_GetEcatMasterSts` and require status `6` (OP).
4. Read EtherCAT master information and require at least two axis resources,
   covering the locked `Axis0` and `Axis1` mapping.
5. Only then release the software emergency configuration, clear axis status,
   enable servos, and home Y followed by X.

No UI action will manually place the master in OP, and no fallback to a
different EtherCAT initialisation sequence is included in this change.

## Failure Behavior

An OP or axis-discovery failure must not issue `IMC_ServoOn`. The existing
failure path will stop axes, close EtherCAT, close the card, and leave the
controller in Fault.

SDK return `0x32000702` must decode its low word `0x0702` as
`ERR_NO_SYS_INT_SIGNAL`, with an action that directs the operator to verify
EtherCAT OP and field-bus connectivity. Numeric error values remain intact.

## Tests

The IMC60G safety test will first demonstrate failures for:

- a successful scan followed by a non-OP master status, with no Servo On;
- fewer than two discovered EtherCAT axes, with no Servo On;
- a wrapped `0x32000702` Servo On result decoded as
  `ERR_NO_SYS_INT_SIGNAL`.

After implementation, the same test executable and the complete printing
test script must pass. Hardware motion is not part of verification.
