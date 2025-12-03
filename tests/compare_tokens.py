#!/usr/bin/env python3
"""
Compare lexer token outputs line by line with a reference file.
Usage: compare_tokens.py <ref-file>
Reads actual tokens from stdin.
"""

import sys
from typing import List


def load_lines(path: str) -> List[str]:
    with open(path, "r", encoding="utf-8") as f:
        return [line.rstrip() for line in f.readlines() if line.strip() != ""]


def load_actual() -> List[str]:
    return [line.rstrip() for line in sys.stdin.readlines() if line.strip() != ""]


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: compare_tokens.py <ref-file>", file=sys.stderr)
        return 2
    ref = load_lines(sys.argv[1])
    actual = load_actual()
    if ref == actual:
        return 0
    print("Token mismatch:", file=sys.stderr)
    print("Expected:", file=sys.stderr)
    print("\n".join(ref), file=sys.stderr)
    print("\nActual:", file=sys.stderr)
    print("\n".join(actual), file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
