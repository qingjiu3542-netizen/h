# Keyboard experiments: legacy avoidance and measured-region docking

## Portrait app-event normalization experiment

SpringBoard crash evidence is now conclusive for the 0.1.14 fullscreen failure: the UUID-matched arm64e dylib aborted through `fvKDDropForHost` at `FVKeyboardDock.inc:228`, where `objectAtIndex:` was called without its loop index. The source now passes the index, and Drop performs no ticket/cache I/O. The app frame evidence shows raw native end frames `(0,554,430,378)` and `(0,932,430,378)` but a missing/unreadable per-app ticket, so there is still no mapped target frame. The app reader now logs open/read/parse/root/field stages to distinguish transport failure from geometry. The expected 9:16 mapping for the 378-point end frame is `(0,386.4,430,378)`; no arbitrary height expansion is applied.

The fullscreen-transition report has no matching SpringBoard crash record in the uploads, so its root cause is not established. Source audit found a real revocation lifetime hazard: associated-object references were cleared before their borrowed values had finished being used. The bridge now snapshots path/Scene/bundle/stamp into a retained work item before clearing, and fullscreen relayout defers LaunchServices/cache revocation until after the layout callback while restoring only keyboard-layer geometry synchronously. This is a targeted mitigation, not proof Safe Mode is fixed; obtain the matching SpringBoard `.ips` to identify the faulting stack. The original tweak scales/moves the whole floating item; its visible keyboard notifications only handle its own search UI, not a universal third-party chat inset. Landscape remains unsupported.


## New keyboard-dock-experiment target (not a verified repair)

FV_KEYBOARD_DOCK_EXPERIMENT builds actual visual writes, separately from both the read-only geometry probe and the legacy whole-item translation below. FVKeyboardDockCore.h derives a uniform scale from host width and a measured, full-width, bottom-docked key rectangle. It never mistakes the full remote canvas height for the key height. Twenty-four pure-C scenarios cover key-height changes, nonzero origins, inner/outer placement, bottom clearance, invalid numbers and rejected floating keyboards. The latest user request removes the experimental extra 6-point inner clearance: runtime now uses zero extra gap, preserving only native keyboard artwork/safe-area padding. FVAspect8_8.log contains 12 applied inner docking records in its last experiment session and the accompanying portrait screenshot shows a complete keyboard; this does NOT verify input-bar avoidance, touch mapping, or any landscape behavior. Landscape was not tested by the user and remains unimplemented. See KEYBOARD-NATIVE-FLAGS.md for the verified original empty-encoding metadata and portrait record-state compatibility patch.

The runtime experiment requires screen-qualified CGRect keyboard notifications, matching source screen/canvas dimensions, a current mount token, active ratio snapshot, confirmed native matched-app show record (with all three Swift flags runtime-validated) or original outer-window container, portrait/nonrotated coordinates, and a layer distinct from the app remote. It writes only that layer's transform/center, preserving the native baseline. Native frame writes, hide, exit, fullscreen and ratio-off restore only still-owned properties. Deferred work is bounded and rejects changed mounts. There is no Swift-address hook, layer migration, global screen-size write, synthetic keyboard notification or scene mutation in this adapter.

CRITICAL LIMITATIONS: Screen-qualified notifications do NOT authenticate the input app or keyboard scene. The original inner flag is only a hint. Delivery in SpringBoard and correspondence of the notification key rectangle to the observed remote surface are NOT device-proven. This code is a CI-only opt-in experiment until association, display and touch mapping are validated. It may conservatively skip all writes. Only portrait runtime geometry is enabled; sideways, split/floating and insufficient-room cases remain native. It does NOT resize app content, update app input insets, move the input bar, implement animation matching, or provide a completed keyboard fix. Outer-window targets may not be discoverable via the existing three-ancestor sampling hook. Do not package this target as a production repair.

## Legacy keyboard-experiment target

The remainder of this document describes FV_KEYBOARD_AVOIDANCE, an opt-in whole-item translation implementation, not a verified input-bar repair or an installable release.

## Preserved original boundaries

- Original FVKeyboardWindow/keyboardView ownership, keyboard-layer migration and original item keyboard state machine remain unchanged.
- No Swift-private address calls. No keyboard layer reparenting, transform assignment, input inset assignment or synthetic native keyboard flags.
- The original item lifecycle handler remains in the didMoveToWindow path. Existing restoration tokens and scene identity checks remain intact.

## Implemented

FV_KEYBOARD_AVOIDANCE enables one process-lifetime notification observer and an NSHashTable weak item registry. Notifications read an array snapshot that holds live items only while handling the event. No per-item observer, item/observer retain cycle or raw global host pointer.

The adapter requires an unambiguous original item with runtime-validated isSideCard=false and innerKeyboardIsDisplay=true. Unknown field types, missing flags, multiple active items, fullscreen, leaving state and disabled ratio fail closed. This uses native state as a conservative hint, not a proven keyboard scene identity.

UIScreen notification source and NSValue CGRect encoding are checked. CGRectValue reads the frame; the screen coordinateSpace converts it to the actual window. Window displacement is converted through the inverse parent-to-window basis before setting item.center. The logical scene ratio and size are not changed by this module.

Repeated notifications recompute from a baseline rather than adding movement. Hide, clear intersection, disabled ratio and pre-detach cleanup undo only the last displacement if this module still owns the center. Native layout or a user drag supersedes the old baseline. An item that cannot fit entirely above the keyboard is left to the native implementation; it is never pushed beyond the viewport top.

The module has no setFrame hook. The already-existing item/host layout drain refreshes avoidance after original layout. The normal item didMoveToWindow handler is preserved. Item willMoveToWindow, item superview changes and host removal clean up before the old relationship is lost. The pre-window hook is installed only in keyboard-experiment. Original outer transforms temporarily retain ownership and disable additional displacement. Fullscreen coverage uses the native baseline, excluding this module's offset to prevent oscillation.

A source-less hide may clear an existing session, but can never create movement. A screen-qualified keyboard frame entirely outside that screen ends the session. Events from another screen cannot replace an active session. These policies are also in the shared production core tests.

## Tests and build

`cc -std=c11 -Wall -Wextra -Werror test-keyboard-avoidance.c -lm -o /tmp/fv-keyboard-tests && /tmp/fv-keyboard-tests`

The shared production core covers intersection, no intersection, offscreen keyboard, insufficient room, nonzero viewport origin, finite-number rejection, all four rotations, ancestor scaling, repeated show/frame-change/hide, partial return on keyboard lowering, and drag/native-layout precedence. Test count is reported by the executable.

Apple CI includes a separate keyboard-experiment target with FV_FIRST_MOUNT_MODE, FV_COMPLETE_MODE and FV_KEYBOARD_AVOIDANCE. FV_DIRECT_ENTRY is intentionally omitted for this target so the original unpatched preference bundle can use the existing Objective-C settings hooks. Other targets do not enable keyboard avoidance.

Apple reference for notification source and screen-to-view conversion:
https://developer.apple.com/documentation/uikit/uiresponder/keyboardframeenduserinfokey
https://developer.apple.com/documentation/uikit/uiresponder/keyboardwillchangeframenotification

## Unresolved and required device checks

- This is whole-item translation. It does not shorten app content or reposition an app-internal input bar. It does not resize a keyboard surface or fix keyboard clipping.
- UIScreen notification objects are documented from iOS 16.1. A missing source cannot initiate movement; a missing-source hide can only clean up existing movement. SpringBoard notification delivery and native flag timing on the target device remain unverified.
- innerKeyboardIsDisplay is a native item state hint, not an authenticated association to a specific keyboard scene. Multiple active items are skipped.
- No animation transaction matching has been added; updates currently apply center directly. Native keyboard and item animation interaction requires device traces.
- Required device cases: show/change/hide, candidate and emoji height changes, interactive dismissal, drag while visible, device and Scene rotation, multi-item focus, close and recreate, fullscreen and return, ratio disable, external display, no observer lifetime leak.
- Current unit tests exercise production C math/state policy. They do not execute UIKit, Objective-C notification dispatch, remote scene rendering, app input insets or real jailbreak loading.

Do not label this as the completed requested 9:16/content-resizing repair. No deb is generated by this work.
