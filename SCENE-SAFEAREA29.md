# 0.1.29 — host-only Scene safe-area A/B/A experiment

## Scope
New independent Apple target `host-safearea-experiment`, compile macro `FV_SCENE_SAFEAREA_EXPERIMENT`. Existing targets remain unchanged when this macro is absent. Package ID remains `com.minis.floatingview.keyboard-host-route-test`, version 0.1.29. Package filter remains SpringBoard + Preferences ONLY. No target-app adapter/injection or blacklist changes.

This is **not demonstrated keyboard avoidance**. The first experiment deliberately uses a small reversible +12 **logical Scene points** bottom inset, NOT keyboard height. No 320->378 correction, no hardcoded keyboard height, no measured user data is compiled into it. Existing dock and host-route diagnostics remain available. The experiment adds no frame, ratio, position, appSetting/cache, display configuration, synthetic keyboard event, whole-item, or app-process mutation. Existing 9:16 geometry including bottom alignment is preserved, as are the previously uncommitted Scene freshness/reentry fixes and their tests.

## What actually changes
For one uniquely matched portrait floating Scene with its native inner keyboard active: A baseline for roughly 2 seconds, B bottom safe-area +12 logical points for roughly 4 seconds, then A restoration. Only one B publication per explicit request; native supersession permanently spends that request. This does not require a diagnostic app. A visible content jump is possible only if the real application honors the changed Scene field. No visible jump does NOT prove the host write failed or that app content received it. Even visible movement is not keyboard-avoidance proof.

## Default-off control
Settings -> floatingView -> 窗口比例 -> `单次安全区 A/B/A（实验）`.
New dedicated preference key `sceneSafeAreaAB29` in `com.minis.floatingview.aspect`; absent means OFF. Enabling creates a new UUID request, disabling removes ONLY this new key. Ratio, app blacklist, original keyboard-mode preferences and all other existing values are left alone. The switch remains ON after a consumed request; toggle OFF then ON to explicitly request another cycle. A SpringBoard restart forgets in-memory consumed requests, so keep the switch OFF during respring/install/uninstall.

1. Install as upgrade over 0.1.28, keep original floatingView installed, respring with the experimental switch OFF.
2. Leave application blacklist intact. Use a permitted app and the existing 9:16 ratio, portrait and native inner-keyboard mode. Keep only one relevant floating host visible.
3. Enable the new switch, then focus a real input field in the floating app. If necessary dismiss/reopen its keyboard so a genuine native callback occurs. Keep device/window orientation and app fixed for the 6-second cycle.
4. Observe A -> B -> A. New log markers `SA29 A1`, `SA29 B`, `SA29 A2`, `SA29 REJECT` appear in the existing FVAspect8 log. A B marker means host-side field readback only, never client delivery or keyboard avoidance.
5. If it never arms, the strict source/Scene/screen/focus/unique-host gate did not authorize a target. If `REJECT absent/unproven field or ABI` appears, **no Scene write** was made. Do not bypass it by forcing explicit zero into settings.
6. To stop: switch OFF. Active runs also stop on hide, focus loss, teardown, Scene replacement, rotation, fullscreen or ratio-off; polling is approximately 100ms with immediate available native hide/teardown/restore hooks.
7. Before uninstall/downgrade: switch OFF, dismiss keyboard, close floating Scene, confirm restoration. If cleanup is unconfirmed or logs report `BLOCKER cleanup-unconfirmed`, close/destroy that Scene and respring before uninstall. Uninstalling a dylib does NOT execute its runtime cleanup automatically.

## Exact interfaces and absence handling
Researched local iOS16 runtime declarations:
- `FBScene -uiSettings` returns id; `-updateSettings:withTransitionContext:completion:` is void with two object arguments and a completion block.
- `-updateUISettingsWithBlock:` exists, but its header exposes only an opaque block; it does NOT prove the callback argument ABI. This experiment **does not call that method** or assume ordinary `scene.settings` is a UIKit subclass.
- `UIMutableApplicationSceneSettings -setSafeAreaInsetsPortrait:` and corresponding getter take/return `UIEdgeInsets` (four doubles on these architectures).
- `BSSettings -objectForSetting:` and `BSMutableSettings -setObject:forSetting:` use an unsigned-long-long setting key.

Primary runtime-header source paths (research only, not vendored):
`https://github.com/qingralf/iOS16-Runtime-Headers/blob/main/PrivateFrameworks/FrontBoard.framework/FBScene.h`
`https://github.com/qingralf/iOS16-Runtime-Headers/blob/main/PrivateFrameworks/UIKitCore.framework/UIMutableApplicationSceneSettings.h`
`https://github.com/qingralf/iOS16-Runtime-Headers/blob/main/PrivateFrameworks/BaseBoard.framework/BSSettings.h`
`https://github.com/qingralf/iOS16-Runtime-Headers/blob/main/PrivateFrameworks/BaseBoard.framework/BSMutableSettings.h`

A local iOS17 getter disassembly suggests `otherSettings` key 15. That is NOT assumed valid for iOS16: before the first publication, a disposable, nonaliasing UIKit mutable settings copy is tested with a nonzero four-component sentinel. The setter, getter, NSValue encoding, field object and unmodified live facade must agree on this runtime. Every private call is ABI/argument-count gated, including UIEdgeInsets, uint64 keys and the completion-block method.

The backing key must already contain a correctly typed NSValue in BOTH fresh ordinary settings and the UI facade, and getter/boxed values must agree. Absent, nil, mismatched or unreadable backing objects are refused. Explicit zero is accepted only if that actual object exists; restore writes explicit zero again. No absent field is materialized; therefore no unsupported unset-vs-zero recovery is needed. This is deliberately narrower than supporting every Scene.

Only a fresh immutable ordinary `scene.settings` snapshot is mutable-copied; only the proven key on its distinct mutable `otherSettings` is changed. Revalidate settings identity, Scene identity, eligibility and expected owned value after every potentially reentrant stage. Publish via runtime-validated ordinary updateSettings. Never replay a saved whole settings snapshot. Restore starts from the freshest settings and changes only the saved original inset if the exact retained Scene still contains our last inset. Other system changes survive; superseded inset values are never overwritten. Reentry cancels further application and routes to guarded restoration. Maximum cycle/cleanup budget is 120 100ms polls or 10 elapsed seconds; unresolved cleanup is reported rather than claiming guaranteed restoration.

## Validation boundaries
New tests execute the real shared transaction core and verbatim native publication/read/mapping-probe/ABI functions with mocked platform boundaries. Cover explicit-zero vs absence, wrong key mapping, copy alias rejection, original-state restore, system supersession, reentry/cancellation, same-value settings replacement, preservation of fresh unrelated fields, no-op, hide/exit restore path, invalid ABI and off-main rejection. Policy tests verify source identity/unique-host gates and teardown/fullscreen/rotation/off/hide wiring. Existing C/Python regressions and arm64/arm64e syntax checks must also pass.

Intended device: iPhone15,3 / iOS 16.4.1(a) / 20E772520a. This build environment cannot run an on-device acceptance test. Runtime private signatures, actual setting presence, host route matching, client relayout, touch alignment, clipping and avoidance remain unverified on that device. If a gate fails, this build honestly rejects the experiment rather than spoofing success.
