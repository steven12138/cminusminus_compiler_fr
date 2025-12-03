#!/usr/bin/env python3
"""
Compare parser traces against a reference, focusing on move steps only.
Reference format follows lab4 .ref files; actual trace is read from stdin.
"""

import sys
from typing import List


def load_moves_from_lines(lines: List[str]) -> List[str]:
    moves: List[str] = []
    for raw in lines:
        line = raw.strip()
        if not line:
            continue
        if line.startswith("define "):
            break  # stop before IR output
        parts = line.split()
        # Expect: <index> <top#lookahead> <action>
        if len(parts) < 3:
            continue
        action = parts[-1]
        if action != "move":
            continue
        entry = " ".join(parts[1:-1])  # drop index and action
        moves.append(entry)
    return moves


def load_moves_from_file(path: str) -> List[str]:
    with open(path, "r", encoding="utf-8") as f:
        return load_moves_from_lines(f.readlines())


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: compare_trace.py <ref-file>", file=sys.stderr)
        return 2

    ref_path = sys.argv[1]
    ref_moves = load_moves_from_file(ref_path)
    actual_moves = load_moves_from_lines(sys.stdin.readlines())

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
