# Portrait app-event normalization experiment

User priority: portrait chat input-bar avoidance first, then full landscape keyboard visibility. Extra six-point bottom clearance is withdrawn. Landscape remains unchanged and unsupported by this portrait experiment.

## Why a new component

The SpringBoard layer transform in 0.1.10 changes only displayed keyboard geometry. Its logs cannot reveal which frame an app receives. The new opt-in experiment adds TWO separately built components: portrait-avoidance-experiment (SpringBoard with a narrow Scene ticket publisher) and keyboard-app-avoidance (application process real-event adapter). Existing dock/probe targets do not enable the publisher and cannot produce app input-bar correction alone.

## Implemented behavior, not device proof

- After verified 9:16 host/remote geometry, the main component publishes an atomic layout ticket for one visible, non-side-card app Scene. It contains Scene identifier, native screen rectangle, verified target dimensions and timestamp; no input text or chat content. It is refreshed on native layout and successful keyboard docking, throttled to two seconds. No polling timer is introduced.
- The app component must independently match UIWindowScene._scene.identifier, exact current key-window bounds and screen, a unique foreground scene, upright portrait, local keyboard, inner mode and a fresh ticket (120s). No frame is altered when these checks fail. File-read sandbox access is NOT assumed to succeed: an unreadable or expired ticket passes all events unchanged.
- The app component overrides the actual class of the DEFAULT notification center only in its process, after checking both original selector ABIs. It forwards each existing postNotificationName:object:userInfo: or postNotification: exactly once. A thread-local guard prevents converting the same nested dispatch twice. Custom notification-center instances pass through unchanged.
- Only real will/did-show and will/did-change-frame keyboard events are considered. A measured full-width, bottom-docked native-canvas frame is relocated to the bottom of the matching app's verified Scene without changing key height. Initial offscreen begin frames use the same coordinate conversion. All unrelated userInfo entries (native animation duration/curve, locality and other payload) survive. Already-corrected frames, floating/split/undocked keyboards, hide events, landscape, other apps/scenes and changed geometry pass unchanged. This does not emit extra keyboard events or change the app's Scene size.
- Main-process fullscreen, ratio-off, outer mode and teardown revoke only the matching Scene's ticket. Removing an installed app component also requires restarting the target app; already injected code cannot be unloaded by replacing package files.

The intended outcome is to let existing app keyboard-notification handlers place their input bar and content area above the ACTUAL displayed keyboard. No generic scroll-view contentInset or third-party input widget is modified. Apps relying exclusively on a different private layout-guide path may not respond. UIKit layout guides, first-event timing, app-side ticket access, hide behavior and WeChat/input-bar behavior are NOT verified by the unit tests. This is NOT a completed input-bar fix.

## Validation evidence available in source

FVKeyboardAppFrameCore.h / test-keyboard-app-frame.c: 33 cases cover the measured 301/378-point family, no repeated conversion, mismatched Scene/windows/size/orientation/mode, expired/future timestamps, nonfinite data, split/undocked keys and initial offscreen geometry. test-keyboard-app-forwarding.py extracts the actual namedHook/noteHook functions and verifies eight real-call forwarding/recursion/ownership cases against native-call mocks. Existing geometry, Swift-field and docking tests remain enabled.

Diagnostic evidence is separate: main log `KB app bridge: portrait Scene ticket published` proves only file publication. App log `<app data container>/Library/Caches/FVKeyboardApp.log` must show `real keyboard event mapped ...`. Installation markers or SpringBoard reservedLogical values do not prove app receipt or input-bar movement. Logs are capped and exclude input text. Neither source tests nor successful Apple compilation substitute for device display/touch validation.

## Packaging boundary

A future test package must explicitly contain the main portrait-avoidance-experiment dylib under the existing SpringBoard/Preferences filter, plus the separate app-only dylib under FVKeyboardAppAvoidance.plist (UIKit bundle filter; implementation skips SpringBoard and Preferences). Do not inject FVIndependent.c's SpringBoard geometry code into all apps. The existing 0.1.10 package contains NEITHER the new app adapter nor the zero-gap changes. This source-only step does not alter an installed device or release a new deb.
