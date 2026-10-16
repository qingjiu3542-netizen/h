#!/usr/bin/env python3
import plistlib
import sys
from pathlib import Path

REQUIRED_ROOT_KEYS = {
    "enabled": bool,
    "triggerEdgeWidth": (int, float),
    "sidebarWidth": (int, float),
    "opacity": (int, float),
    "positionYRatio": (int, float),
    "apps": list,
}

REQUIRED_APP_KEYS = {
    "bundleID": str,
    "displayName": str,
    "urlScheme": str,
}


def fail(message: str) -> int:
    print(f"[config-check] ERROR: {message}")
    return 1


def validate(path: Path) -> int:
    if not path.exists():
        return fail(f"missing file: {path}")

    with path.open("rb") as handle:
        data = plistlib.load(handle)

    if not isinstance(data, dict):
        return fail("root object must be dictionary")

    for key, expected in REQUIRED_ROOT_KEYS.items():
        if key not in data:
            return fail(f"missing root key: {key}")
        if not isinstance(data[key], expected):
            return fail(f"root key '{key}' has wrong type")

    opacity = float(data["opacity"])
    if not (0.0 < opacity <= 1.0):
        return fail("opacity must be in (0, 1]")

    for index, app in enumerate(data["apps"]):
        if not isinstance(app, dict):
            return fail(f"apps[{index}] must be a dict")
        for key, expected in REQUIRED_APP_KEYS.items():
            if key not in app:
                return fail(f"apps[{index}] missing '{key}'")
            if not isinstance(app[key], expected) or not app[key].strip():
                return fail(f"apps[{index}] invalid '{key}'")

    print(f"[config-check] OK: {path}")
    return 0


if __name__ == "__main__":
    input_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("layout/var/jb/Library/SidebarLauncher/config.plist")
    raise SystemExit(validate(input_path))
