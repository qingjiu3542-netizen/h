# Original keyboard state compatibility audit

Source: original floatingView 1.6.0-6 fat dylib, SHA256 947d8215c3aed79ccaacc7cee413ec8cafb50460ac7199fa7eef24d38ea5ac46. No original binary is included here.

Both slices were parsed directly from their Objective-C ivar lists, including name, offset-cell value, type string, alignment and size. ARM64 class file offset 0xa5cf0; ARM64e class file offset 0xb1cf0. Both identify `_TtC12floatingView17FVAppDispatchItem`.

| Property | Initial offset cell value | Type string | Alignment exponent | Size |
|---|---:|---|---:|---:|
| isSideCard | 120 | empty string | 0 | 1 |
| innerKeyboardIsDisplay | 152 | empty string | 0 | 1 |
| recordOnlyInnerKeyboardIsShow | 153 | empty string | 0 | 1 |

These numbers describe the input binary, NOT offsets to hardcode into a live object. UIKit/Swift non-fragile layout can adjust the superclass size. The adapter resolves all three ivars at runtime, validates the exact original class, superclass/instance bounds, relative spacing (+32 and +1), allowed empty/B/c encodings, and all three 0/1 values. Unknown names, NULL encoding, changed layout or invalid values fail closed. No state field is written.

ARM64 disassembly (not callable ARM64e addresses):
- The show-state path compares the item's bundleIdentifier at 0x42c40 onward.
- On a matched show, 0x42c78 loads the record flag offset cell (0xabce8) and 0x42c80 stores 1.
- 0x42c98 loads the global orientation owner, +0x30 at 0x42c9c. 0x42ca0..0x42ca8 exits this branch for orientation values 1/2, before later inner-transform processing.
- 0x42d0c loads the inner flag offset cell (0xabce0), a subsequent state used to avoid repeating item transformations.
- Therefore `innerKeyboardIsDisplay == 1` is not a sound required visibility predicate for this portrait branch. For the portrait-only experiment use `isSideCard == 0` and the matched-show record == 1, with inner still constrained to a valid bool. Orientation 2 remains unsupported in the experiment for independent geometry reasons.

This fixes two concrete compatibility errors in 0.1.9: empty Swift encodings were rejected, and a subsequent transform flag was treated as a universal portrait visibility flag. It does not prove runtime notification/surface association, touch mapping, full keyboard visibility, or app input-bar avoidance. Other native branches can still legitimately leave the record false; those cases remain rejected rather than forced.

Bottom clearance: inner experiment computes 6 window-coordinate points converted into the keyboard parent's basis. Space checks and reservedLogicalHeight include that clearance. Outer behavior is unchanged. reservedLogicalHeight remains a computed diagnostic value, not an applied app inset.

Tests: test-keyboard-native-flags.c validates layouts, states, changed class/offset/encoding and corrupted values; test-keyboard-native-adapter.py compiles the actual fvKDFlag runtime function against mocked Objective-C runtime metadata. Neither substitutes for device testing.
