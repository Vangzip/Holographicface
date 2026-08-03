# Non-blocking Camera Startup Design

## Goal

Prevent the capture window from becoming unusable when the vendor camera SDK blocks during startup, while preserving the existing live-preview behavior once initialization succeeds.

## Evidence and scope

On the connected `IDG-2600-C-5G`, `mergeholo.exe --capture` opens the camera and captures successfully. The GUI window is responsive but remains on `初始化相机` after 30 seconds. `CaptureWindow::startCamera()` calls `LightFieldCapture::initialize()` synchronously from a `QTimer` callback on the GUI thread, so a vendor-SDK wait prevents all subsequent UI-state updates.

This change is limited to camera-startup lifecycle handling in `CaptureWindow`; the camera configuration format, frame acquisition, and pipeline behavior remain unchanged.

## Design

`CaptureWindow` will own a dedicated initialization thread for each camera-start attempt. The thread creates and initializes `LightFieldCapture` away from the GUI thread, then posts exactly one completion result back to the window.

The window retains its `Starting` state while work is pending and uses a 15-second watchdog. At the watchdog deadline it returns to `Error`, reports that startup is taking too long, and restores the settings button. A pending vendor call is not force-terminated because the SDK exposes no safe cancellation mechanism. Until that call finishes, the window rejects another start attempt, preventing two camera handles from opening concurrently.

If initialization completes successfully, ownership of the initialized capture object transfers to the GUI thread, `Live` state begins, and the existing frame timer starts. If it fails, the window reports a concrete initialization failure and releases the failed object. A late completion after timeout is released without changing the user-visible state.

Window destruction waits for a completed initialization thread only when it has returned; it never dereferences a destroyed window from the worker. The close action tells the user that an SDK startup call is still pending if it cannot close immediately.

## Error handling

- Start failure: transition to `Error` and show the existing initialization error message.
- 15-second wait: transition to `Error`, retain a distinct `initializationPending` guard, and show an actionable timeout message.
- Successful completion after timeout: release the initialized camera without starting preview, so the user can safely reopen the program after the vendor call returns.
- A capture error after successful startup continues to use the existing `pollCameraFrame()` error path.

## Testing

Extract the GUI-independent state transitions into a small camera-start controller that accepts completion events. Tests will verify: startup immediately returns control, success reaches live state, failure reaches error state, timeout enables recovery without allowing a duplicate initialization, and late completion is discarded safely. Existing settings tests and the release executable will be rebuilt and run. Finally, an automated GUI probe will verify that the status changes out of `初始化相机` by the timeout rather than remaining indefinitely.
