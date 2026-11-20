# Host/system-side keyboard routing research — no target-app injection

## Required behavior

The user explicitly chooses the host/system-side route: target apps may remain in their injection blacklist. The selected portrait 9:16 layout must remain stable; bottom input bars and app content should respond natively. Do not temporarily restore the original aspect, resize/move the entire floating item, force app injection, globally alter insets, or relabel diagnostic builds as fixes. The user reports original-ratio behavior works across apps even while blacklisted; this is a critical baseline observation, not proof of a particular private API.

## Current implementation limitation

0.1.20 has a SpringBoard visual docking component and a separate target-app notification adapter. The latter cannot execute when the loader excludes the target process. The current design therefore cannot satisfy this chosen requirement merely by changing its app identity filter. Retain the validated implementation for comparison; do not silently remove it from existing installed packages or claim it is host-only.

## Verified code-level difference

FVGeometry.inc fvGSceneFrame changes app Scene settings.frame with updateSettings:withTransitionContext:completion:, then checks the host-side settings frame and updates cached appSetting.frame. Its geometry transaction separately changes parent bounds, host frame, remote bounds/transform/center. This proves neither client keyboard coordinate consistency nor a matching display-reference configuration. Visual keyboard docking separately uses the original screen canvas and measured keys. A reduced app notification height is numerically close to the original-canvas intersection with the smaller viewport, but the producing layer remains unlocated.

Original arm64 audit identifies a real UIKeyboardSceneDelegate _showKeyboardWindowForInputViewSet:placement:windowScene: hook. Its ObjC original IMP receives objects inputViewSet, placement and windowScene and returns BOOL. 0x6c194 is a Swift wrapper, not a callable C IMP. 0x6c318–0x6c32c bridges and forwards; 0x6c938 registration and 0x73d28 store the original IMP in 0xad2b0. Addresses are static arm64 evidence, never arm64e runtime addresses. Original whole-item state machine and keyboard-layer migration are not generic app input-bar relayout.

## Narrow research targets

1. System show/placement original call and its exact target Scene: establish whether its placement/owner coordinates affect keyboard occlusion delivered to the target or merely keyboard-window presentation.
2. Active keyboard Scene delegate/inputViews: original 0x203b4 path reads activeKeyboardSceneDelegate and inputViews. This proves a read path, not a server-side geometry setter.
3. Keyboard service/scene to target app scene state delivery: identify where full key position is clipped/converted before the first UIKit event, preserving session identity, coordinate references, transition timing and hidden state. Pure SB cannot directly edit an already-created in-process app notification object without a communication/OS delivery path.

## Additional header checks

Downloaded nst/iOS-Runtime-Headers primary runtime dumps for FBSSceneSettings and UIInputViewSetPlacement for static interface inspection, not device ABI validation. FBSSceneSettings exposes frame, displayConfiguration, displayIdentity and occlusions; names alone do not prove keyboard semantics. Do NOT change generic scene occlusions or display configuration based only on names. UIInputViewSetPlacement exposes extendedHeight, applicatorInfoForOwner:, constraints, and remoteIntrinsicContentSizeForInputViewInSet:includingIAV:; it does not expose a demonstrated writable complete screen keyboard rect. Do NOT force extendedHeight as a substitute for a verified system-side occlusion update.

## Required comparison and acceptance

Use the same blacklisted target, orientation, keyboard type and input bar. Capture host-only native-ratio versus 9:16 session/Scene, source and destination coordinate references, actual keyboard state handoff and existing presentation. Derive a correction only after proving a writable downstream consumer before the first app layout event. Runtime ABI validation and exact ownership/session checks remain mandatory. Validate visible app-internal bar movement, content relayout, first-frame sync, candidate/emoji changes and hide restoration while item presentation remains fixed. Cross-app tests are necessary; generic filters are not proof of generic behavior.

No functional host-only patch or new package has been produced by this research pass. No device test has established a server-side correction yet. Existing 0.1.18 visible WeChat height success and later sync/QQ failures remain the actual evidence.

## Build and compare original host path without app injection

A dedicated `host-route-probe` build was added under `FV_KEYBOARD_HOST_ROUTE_PROBE`, independent of the app adapter, cache ticket and injection filter. It observes only runtime-ABI-matched UIKit show/placement, scene-delegate arbiter update, remote-keyboard prepare, and the documented UIKit arbitration changed-state sender/queue if present. It records original input rect and IAV rect, flags, receiver class, a non-reversible hash of the supplied Scene identifier and currently attached floating-host records (target bundle, hashed host Scene, Scene settings frame, cached appSetting frame, remote bounds/mount identity). It never emits app chat text, scene IDs or data-container paths. It holds live hosts in a weak table. An observation occurs only on the main thread.

Each compatible original IMP is forwarded exactly once with the same CGRect, BOOL, object, completion and return. Per-route recursion bits let distinct stages in one nested native chain all be observed while preventing a same-route callback from recursively consuming its own record. The probe does not mutate placement, Scene, keyboard state, focus, status bars, occlusions, geometry, notification contents or fences. It performs no delayed work. This is deliberately a no-write comparison instrument, not the fix.

Required test: in the same target app and keyboard type, compare original ratio and 9:16 while the app remains blacklisted. Correlate show parameter rects, subsequent arbiter state rects and Scene hashes with host frame/settings/cached-appSetting/remote bounds; identify the first stage where 378pts becomes ~210/178pts or Scene ownership diverges. If no particular endpoint has a matching ABI it is not hooked. No route probes can execute, installation filters differ, or logs show only geometry but not app-client change? These are distinct failure conclusions.

Pure host-side production correction is not yet implemented. Only after the measured discrepancy is located at a documented writable upstream handoff should the proven geometry mapping be applied there; otherwise preserve native original ratio behavior. User-facing chatbar response cannot be claimed solely from a matching rectangle log.
