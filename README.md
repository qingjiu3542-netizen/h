# floatingView aspect research

Experimental source extension for floatingView. Apple/Xcode CI validates arm64 and arm64e without rewriting Mach-O headers. This repository contains no original plugin binary, credentials, or device crash reports.

## Status

Not a completed 16:9 repair. First-mount Scene submission and restoration are experimental. Client first-frame layout, landscape integration and keyboard avoidance are not verified. CI produces validation dylibs and evidence, not an installable deb. Do not install artifacts on a daily-use device.

## Build

Run the macOS Actions workflow or `python3 ci/apple-build.py` under Xcode with the iPhoneOS SDK. The build fails for incorrect CPU subtype, unresolved symbols or failed tests/signature checks.
