# Keyboard arbitration: protocol topology correction

## Scope and evidence

Read-only examination of existing RuntimeBrowser declarations in `/tmp/fv-host-keyboard-headers/` on 2026-11-21. These are static interface declarations, not target-device implementation traces. No system UIKitCore implementation binary is present in the audited materials. No hooks, production geometry or packaging changed.

## 1. Do not portray the observations as a single sequential IPC trace

`_UIKeyboardArbitration.h` declares service-facing operations:

- `signalKeyboardChanged:onCompletion:` (comment identifies `_UIKeyboardChangedInformation *`).
- `startArbitrationWithExpectedState:focusContext:hostingPIDs:usingFence:withSuppression:onConnected:`.
- `setClientFocusContext:`.
- `setWindowHostingPID:active:`.

`_UIKeyboardArbitrationClient.h` declares callbacks:

- `queue_keyboardChanged:onComplete:`.
- `queue_sceneBecameFocused:withCompletion:` (comment identifies `FBSSceneIdentityToken *`).
- `queue_keyboardIAVChanged:onComplete:`.
- `queue_keyboardUIDidChange:onComplete:`.

The conceptual protocol boundary is therefore:

```text
client-side producer
  -- service-facing signalKeyboardChanged --> arbitration service
client-side receiver
  <-- callback queue_keyboardChanged ------- arbitration service
```

This is an interface topology, NOT a proven implementation order, sender PID, service name, fan-out rule, or target-specific routing trace. In particular a callback observed inside SpringBoard can be SpringBoard receiving its own relevant arbitration event; it does not establish that SpringBoard is forwarding that callback to WeChat. A local wrapper can also call a protocol method directly. Transport use at a particular invocation requires implementation or trace evidence.

`_UIRemoteKeyboards` conforms to both `_UIKeyboardArbiterClient` and `_UIKeyboardArbitrationClient`, declares an `NSXPCConnection *connection`, and exposes a `_UIKeyboardArbitration` proxy. `_UIKeyboardArbiterClient` also declares an NSXPCConnection. This supports looking for a service boundary, but does not identify the service implementation or which connection reaches which process.

## 2. Target identity is not stored in the declared change object

`_UIKeyboardChangedInformation.h` lists only keyboardPosition, keyboardPositionWithIAV, keyboardOnScreen, animationFence. It supports NSCopying/NSSecureCoding and declares encodeWithCoder/initWithCoder. No target PID, scene token or bundle identifier appears among its declared fields/properties.

Consequences:

- A local state pointer token is not a recipient identifier.
- A pointer value cannot be expected to identify a deserialized object in another process.
- Recipient binding must be established from connection/focus/hosting context or implementation outside this declared payload. This is a requirement for further proof, not a claim about exact routing internals.
- Constructing a replacement information object from a measured dock rectangle would not itself bind it to the correct app.
- Preserve fence and IAV semantics; copyWithoutFence/copyWithoutIAV exist but do not justify using them for a geometry fix.

## 3. Hosted identities are explicitly separate from change geometry

`_UIKeyboardArbiterClientInputDestination.h` declares `currentHostedPIDs`, `addWindowHostingPID:`, `removeWindowHostingPID:`, `setWindowHostingPID:active:`, `isHostedInAnotherProcess`, `focusedSceneIdentityStringOrIdentifier`, `prepareForHostedWindowWithScene:`, and `queue_sceneBecameFocused:withCompletion:`.

These are specific identity/hosting leads. Their declarations do not prove that the custom 9:16 path omitted registration, that a hosting PID is the keyboard recipient, or that calling prepareForHostedWindowWithScene would repair it. Do not synthesize focus, add PIDs, or replace requiredScene based only on these names.

## 4. RuntimeBrowser ABI warnings matter

The protocol dumps mark several block-bearing methods as `void *` and report argument-type parse warnings. The comments are useful semantic leads, not hook-ready encodings. In particular startArbitration mixes object, bool, int and block parameters. Never derive a callable prototype from the apparent all-void-pointer declaration. A device runtime signature or independently verified implementation ABI is required before any hook/call.

## 5. Revised interpretation of build39

The same-process route4 -> route6 state-token adjacency is evidence for a local receive/handle segment. It is not an observed outgoing per-WeChat transaction. The next missing static evidence is the arbitration service's handling of signalKeyboardChanged and selection of callback connections, plus the target UIKit callback consumer and geometry conversion. Static headers alone cannot supply either implementation.

The 378-point surface measurement and 764.444-point app viewport still give the device-validated app-side top of 386.444. Neither arithmetic nor secure-coding support establishes where that number can safely be written on the service side. No new package or repeated device test is warranted from these declarations alone.
