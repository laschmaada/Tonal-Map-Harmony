#!/usr/bin/env python3
"""Patch a JUCE-generated moduleinfo.json so VST3 hosts that read Class Flags
classify the plugin as a MIDI effect.

Background
----------
JUCE 8's _juce_configure_plugin_targets / JUCEUtils.cmake does not translate
`IS_MIDI_EFFECT TRUE` (which sets `JucePlugin_IsMidiEffect=1`) into the
per-class `Class Flags` field of the VST3 `moduleinfo.json`. Without that
flag, hosts like Ableton Live 12 treat the plugin as a generic Audio FX and
refuse to place it before an instrument on a MIDI track.

This script rewrites `Class Flags` to 2 (`kClassFlagsMidiEffect`) on every
class whose `Category` is `Audio Module Class` or
`Component Controller Class`. The Plugin Compatibility Class is left alone
(it does not advertise a behaviour to the host).

Usage
-----
    python patch_moduleinfo.py <path/to/moduleinfo.json>

Exit status: 0 on success, non-zero on error. The script is idempotent and
safe to re-run after every build.
"""
import json
import re
import sys
from pathlib import Path

# JUCE 8's juce_vst3_helper writes moduleinfo.json with a trailing comma after
# every value, e.g. `"Flags": { ... },` and `"Snapshots": [],`. Python's
# stock json module rejects those, so we strip them before parsing.
_TRAILING_COMMA_RE = re.compile(r",(\s*[\]}])")


def _strip_trailing_commas(text: str) -> str:
    return _TRAILING_COMMA_RE.sub(r"\1", text)

# kVstClassFlagsMidiEffect (VST3 spec, steinberg/vst3sdk/pluginterfaces/vst/vsttypes.h)
MIDI_EFFECT_CLASS_FLAGS = 2

TARGET_CATEGORIES = frozenset({
    "Audio Module Class",
    "Component Controller Class",
})


def patch(path: Path) -> int:
    if not path.is_file():
        print(f"[patch_moduleinfo] ERROR: not a file: {path}", file=sys.stderr)
        return 1

    try:
        with path.open("r", encoding="utf-8") as f:
            raw = f.read()
    except OSError as exc:
        print(f"[patch_moduleinfo] ERROR: cannot read {path}: {exc}", file=sys.stderr)
        return 1

    # JUCE emits trailing commas; strip them so json.load accepts the file.
    try:
        data = json.loads(_strip_trailing_commas(raw))
    except json.JSONDecodeError as exc:
        print(f"[patch_moduleinfo] ERROR: cannot parse {path}: {exc}", file=sys.stderr)
        return 1

    classes = data.get("Classes")
    if not isinstance(classes, list):
        print(f"[patch_moduleinfo] ERROR: no 'Classes' list in {path}", file=sys.stderr)
        return 1

    touched = 0
    for cls in classes:
        if not isinstance(cls, dict):
            continue
        if cls.get("Category") in TARGET_CATEGORIES:
            if cls.get("Class Flags") != MIDI_EFFECT_CLASS_FLAGS:
                cls["Class Flags"] = MIDI_EFFECT_CLASS_FLAGS
                touched += 1

    try:
        with path.open("w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")
    except OSError as exc:
        print(f"[patch_moduleinfo] ERROR: cannot write {path}: {exc}", file=sys.stderr)
        return 1

    print(
        f"[patch_moduleinfo] Patched {path.name}: "
        f"Class Flags set to {MIDI_EFFECT_CLASS_FLAGS} on "
        f"Audio Module and Component Controller "
        f"({touched} class(es) updated)."
    )
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {argv[0]} <moduleinfo.json>", file=sys.stderr)
        return 2
    return patch(Path(argv[1]))


if __name__ == "__main__":
    sys.exit(main(sys.argv))
