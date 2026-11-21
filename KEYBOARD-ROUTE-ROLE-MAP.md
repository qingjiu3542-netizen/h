# Keyboard route role map (read-only, 2026-11-21)

## Evidential labels

- **Declared**: appears in local iOS 16 RuntimeBrowser interface dump.
- **Observed**: covered by current host-route probe/logs.
- **Candidate**: semantically plausible next boundary, but no implementation/call trace proves it.
- **Not established**: cannot be used as a production write point yet.

The header dump is not an iOS implementation binary. It gives signatures and protocol roles, not selector call ordering. Host-route hooks call each original implementation once and preserve arguments/return; they are read-only.

## Role-separated map

| Stage | Interface / method | What the declaration means | Status for the 9:16 issue |
|---|---|---|---|
| Keyboard scene producer | `UIKeyboardSceneDelegate updateArbiterClientWithKeyboardFrame:keyboarFrameWithIAV:isIAVRelevant:showing:notifyRemote:forScene:` | Receives two CGRects, three BOOL flags, and a Scene-like object; a plausible place where scene-local keyboard geometry is supplied to arbiter client. | Hook ABI was installed in build39, but **zero calls were logged** in the submitted SpringBoard session. Therefore its arguments are not available from that test; a call may occur in another process/context or not on this path. |
| Remote movement preparation | `_UIRemoteKeyboards prepareToMoveKeyboard:withIAV:isIAVRelevant:showing:notifyRemote:forScene:` | Remote keyboard movement input with explicit scene parameter. | Observed as `prepare-keyboard-scene`; 42 records (13 native, 29 ratio9x16) in the ABI-fix session. Some show `(0,0,430,0)` with notify false; this is not the complete 378-point app keyboard frame. |
| Client -> arbiter service | `_UIKeyboardArbitration signalKeyboardChanged:onCompletion:` | Protocol declares state object sent through a service-facing signal. | The similarly named `signalToProxyKeyboardChanged:onCompletion:` hook installed, but zero `signal-arbiter-state` records were logged. Static declaration alone does not prove transport. |
| Arbiter service -> client callback | `_UIKeyboardArbitrationClient queue_keyboardChanged:onComplete:` | Protocol declares a queued client callback carrying `_UIKeyboardChangedInformation`. | Worker route-4 observations correlate by receiver/state pointer token with main-thread route-6 handler calls. This is a local callback/handle association only; pointer token is non-authorizing and does not prove app delivery. |
| Input destination state handling | `_UIKeyboardArbiterClientInputDestination _didHandleKeyboardClientChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:`; `inputDestinationDidChange:...forScene:`; `prepareForHostedWindowWithScene:` | Class declares destination/focus/hosted PID concepts and scene-specific update methods. | Partially observed: did-handle and input-destination change hooks exist. But no logged recipient PID or verified mapping from hosted PID to WeChat UIWindowScene. Do not force registration/focus. |
| Input UI-host presentation | `_UIKeyboardArbiterClientInputUIHost keyboardVisibilityDidChangeWithFrame:visible:tracking:`; `completeKeyboardChange`; `_layoutKeyboardViews:` | A separate input-UI-host subclass handles keyboard visibility/presentation frame. | The new selector is now added to the local, unshipped read-only probe source with strict direct-method/argument-count/type-encoding checks. Build39 did not hook it. It is a candidate local UI-host surface consumer, not known to be app layout recipient. |
| State payload | `_UIKeyboardChangedInformation` | Secure-codable/copyable state with keyboardPosition, keyboardPositionWithIAV, keyboardOnScreen, animation fence. | Correction: the old local dump was mixed-version. Public iOS16 declarations expose host/source bundle, display and sourceSceneIdentityString fields, plus remote-IAV geometry. These source/host hints do not authenticate an intended recipient; runtime ABI and result types must be checked. |
| App UIKit consumer | UIKit app Scene / keyboard notifications and layout | Would have to apply the event to the target application’s own coordinate space and layout. | Not observed in SpringBoard-only logs, and target app is excluded from injection. The final consumer and transform remain unlocated. |

## Critical direction correction

The conceptual direction is two-sided, not a proven linear SpringBoard call chain:

```text
producer/client -- signal/proxy --> arbitration service
client/receiver <-- queued callback -- arbitration service
```

A probe line for `queue_keyboardChanged` is an inbound callback in that interface model. A `signalToProxyKeyboardChanged` line is a local proxy invocation candidate. Same-process sequence adjacency and raw object-token equality do not prove they are a matched IPC request/reply. Do not describe route 4 -> route 6 as a transaction unless a stable transaction identifier or service trace establishes it.

## Build39 log reanalysis and source-only diagnostic delta

The latest `abi-fix-1` segment in `FVAspect8_23.log` contains 126 `KHROUTE` records: 42 each for `show-window-scene`, `prepare-keyboard-scene`, and `remote-did-handle-change` (29 ratio9x16 and 13 native in each route). The update-arbiter selector, signal-to-proxy wrapper, queued-state main-thread wrapper, and input-destination change wrapper were ABI-installed, but there are **zero corresponding `KHROUTE` invocation records** in that segment. The actual producer tuple is therefore absent from the device evidence; installation success must not be presented as a call trace.

The route-4 worker records and route-6 remote handler sometimes share receiver/state pointer-value tokens, but no stable transaction ID is logged. They show a local association candidate, not a proven request/reply or an app handoff. The attached host still independently logs a full 378-point dock at y=386.4 and `inputInsetImplemented=0`.

To continue source work without changing production geometry, `FVKeyboardHostRouteProbe.inc` now has a **local, unshipped** observer for `_UIKeyboardArbiterClientInputUIHost keyboardVisibilityDidChangeWithFrame:visible:tracking:`. It is guarded by the existing exact-encoding/direct-method installer and will reject missing or incompatible runtime ABI. It records only frame, visibility/tracking BOOLs, receiver class, process and mode; it never reads target-app getters or changes an argument/return. Sampling is separately bounded for visible and hidden events and mode. The hook is diagnostic only and will not establish app layout delivery by itself.

Tests added:

- mock installer checks the selector's CGRect+BOOL ABI, argument count, direct ownership and idempotence;
- actual wrapper test checks exact one-call forwarding, full CGRect/BOOL preservation, native/9:16 labeling, nested calls and worker-safe scalar logging;
- full existing host-route hook suite rerun.

No device runtime encoding has been confirmed for this new method, no Apple build/CI or package has been made, and the code has not been installed. Any new test should be requested only if the user wants to validate this diagnostic build; no geometry fix is implied.

## Remaining recipient-boundary gap

`_UIKeyboardArbiterClientInputDestination _layoutKeyboardViews:` remains a plausible next inspection point, but adding it is not yet justified: current logs have no input-destination call and the class header alone does not establish the concrete implementation signature. First verify the live method's class ownership/encoding and whether `currentHostedPIDs` actually contains the target app; do not log raw PID lists unless required. The UIHost callback test can at least establish whether that presentation stage receives the same full frame as the dock. If so, it reinforces that the remaining issue lies at app-scene delivery/layout; if not, it localizes a geometry discrepancy earlier without guessing a setter.

## Conclusion and decision

The current evidence justifies no geometry patch and no production package. The known 378 -> 386.444 app-side conversion remains verified, while the all-app host-only requirement needs a proven recipient-bound handoff and coordinate transform. Do not ask the user to repeat the unchanged native/9:16 test; request a new diagnostic test only if the user chooses to install the source-built observer and the result will alter the next implementation decision.

