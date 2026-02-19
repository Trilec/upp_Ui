#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


FORBIDDEN = [
    (re.compile(r"\bRefreshLayout\s*\("), "RefreshLayout() in Paint"),
    (re.compile(r"\bEnsure[A-Za-z0-9_]*Cache\s*\("), "Ensure*Cache() in Paint"),
    (re.compile(r"\bUiCompute[A-Za-z0-9_]*Layout\s*\("), "UiCompute*Layout() in Paint"),
    (re.compile(r"\bLayout\s*\("), "Layout() call in Paint"),
]


def find_paint_bodies(text: str) -> list[tuple[int, str]]:
    out: list[tuple[int, str]] = []
    sig = re.compile(r"\b(?:[A-Za-z_][A-Za-z0-9_:<>~]*\s+)*[A-Za-z_][A-Za-z0-9_:<>~]*::Paint\s*\([^\)]*\)\s*\{")
    for m in sig.finditer(text):
        start = m.end() - 1
        depth = 0
        i = start
        n = len(text)
        while i < n:
            ch = text[i]
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    body = text[start + 1 : i]
                    line = text.count("\n", 0, m.start()) + 1
                    out.append((line, body))
                    break
            i += 1
    return out


def main() -> int:
    repo = Path(__file__).resolve().parent.parent
    ui_dir = repo / "Ui"
    files = sorted(ui_dir.glob("*.cpp"))
    violations: list[str] = []

    for fp in files:
        text = fp.read_text(encoding="utf-8", errors="ignore")
        for line, body in find_paint_bodies(text):
            for rx, label in FORBIDDEN:
                mm = rx.search(body)
                if mm:
                    body_line = body.count("\n", 0, mm.start())
                    violations.append(f"{fp.as_posix()}:{line + body_line}: {label}")

    if violations:
        print("Paint purity check failed:")
        for v in violations:
            print(f"- {v}")
        return 1

    print("Paint purity check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
