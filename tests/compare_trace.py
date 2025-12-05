#!/usr/bin/env python3
"""
Compare parser traces against a reference, ignoring move steps.
Reference format follows lab4 .ref files; actual trace is read from stdin.
Assumes parser output only (no IR), e.g. via --gtrace-only.
"""

import sys
from typing import List


def load_steps_from_lines(lines: List[str]) -> List[str]:
    steps: List[str] = []
    for raw in lines:
        line = raw.strip()
        if not line:
            continue
        parts = line.split()
        # Expect: <index> <top#lookahead> <action>
        if len(parts) < 3:
            continue
        action = parts[-1]
        if action != "move":
            continue  # ignore move steps for comparison
        entry = " ".join(parts[1:])  # drop index; keep rest including action
        steps.append(entry)
    return steps


def load_moves_from_file(path: str) -> List[str]:
    with open(path, "r", encoding="utf-8") as f:
        return load_steps_from_lines(f.readlines())


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: compare_trace.py <ref-file>", file=sys.stderr)
        return 2

    ref_path = sys.argv[1]
    ref_moves = load_moves_from_file(ref_path)
    actual_moves = load_steps_from_lines(sys.stdin.readlines())

    if ref_moves == actual_moves:
        return 0

    print("Trace mismatch:")
    print("Expected moves:", file=sys.stderr)
    print("\n".join(ref_moves), file=sys.stderr)
    print("\nActual moves:", file=sys.stderr)
    print("\n".join(actual_moves), file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
