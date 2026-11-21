#!/usr/bin/env python3
"""Checks that the repository has everything a builder or contributor needs.

1. Every required file exists and is not empty.
2. Every STL is a readable STL (ASCII or binary) with at least one triangle.
3. Every relative link in the Markdown files points at a file that exists,
   and every #anchor points at a real heading.
4. Every issue-template link (?template=foo.yml) points at a real template.

Run it from anywhere:  ./scripts/check_repo.py
Exits non-zero and prints what is wrong if anything fails.
"""
import os
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPO_SLUG = "codingsushi79/openchessclock"

# Add new files here when they become part of the build or the contributor flow.
REQUIRED_FILES = [
    # docs
    "README.md",
    "PARTS.md",
    "WIRING.md",
    "ASSEMBLY.md",
    "CONTRIBUTING.md",
    "CODE_OF_CONDUCT.md",
    "CHANGELOG.md",
    "LICENSE",
    # firmware + simulator
    "firmware/chess_clock_pico/chess_clock_pico.ino",
    "test/run_tests.sh",
    "test/harness.cpp",
    "test/stubs/Arduino.h",
    "test/stubs/Adafruit_SharpMem.h",
    # CAD + printable parts
    "cad/chess_clock.scad",
    "stl/chess_clock_body.stl",
    "stl/chess_clock_lever.stl",
    "stl/chess_clock_plate.stl",
    "stl/chess_clock_caps.stl",
    # images used by the docs
    "images/hero.png",
    "images/screen_layouts.png",
    # GitHub community files
    ".github/PULL_REQUEST_TEMPLATE.md",
    ".github/ISSUE_TEMPLATE/config.yml",
    ".github/ISSUE_TEMPLATE/firmware_bug.yml",
    ".github/ISSUE_TEMPLATE/model_issue.yml",
    ".github/ISSUE_TEMPLATE/docs_issue.yml",
    ".github/ISSUE_TEMPLATE/feature_request.yml",
    ".github/ISSUE_TEMPLATE/build_report.yml",
]

SKIP_DIRS = {".git", "node_modules", "gfx", "shots", "build"}

errors = []


def err(msg):
    errors.append(msg)


def check_required():
    for rel in REQUIRED_FILES:
        p = ROOT / rel
        if not p.is_file():
            err(f"missing required file: {rel}")
        elif p.stat().st_size == 0:
            err(f"required file is empty: {rel}")


def stl_triangles(path):
    data = path.read_bytes()
    if len(data) >= 84:
        (n,) = struct.unpack_from("<I", data, 80)
        if n > 0 and len(data) == 84 + 50 * n:
            return n  # binary STL
    if data.lstrip().startswith(b"solid"):
        return data.count(b"facet normal")  # ASCII STL
    return 0


def check_stls():
    for p in sorted((ROOT / "stl").glob("*.stl")):
        if stl_triangles(p) == 0:
            err(f"{p.relative_to(ROOT)} is not a valid STL or has no triangles")


def slug(heading):
    h = re.sub(r"[`*_]|<[^>]+>", "", heading).strip().lower()
    h = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", h)
    h = re.sub(r"[^\w\- ]", "", h)
    return h.replace(" ", "-")


def anchors_of(md_path, cache={}):
    if md_path not in cache:
        found, counts, in_code = set(), {}, False
        for line in md_path.read_text(encoding="utf-8").splitlines():
            if line.lstrip().startswith("```"):
                in_code = not in_code
                continue
            m = None if in_code else re.match(r"#{1,6}\s+(.*?)\s*#*\s*$", line)
            if m:
                s = slug(m.group(1))
                n = counts.get(s, 0)
                counts[s] = n + 1
                found.add(s if n == 0 else f"{s}-{n}")
        cache[md_path] = found
    return cache[md_path]


LINK_RE = re.compile(r"(?<!!)\[[^\]]*\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)|!\[[^\]]*\]\(([^)\s]+)\)")


def markdown_files():
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for f in filenames:
            if f.endswith(".md"):
                yield Path(dirpath) / f


def check_links():
    template_dir = ROOT / ".github" / "ISSUE_TEMPLATE"
    for md in markdown_files():
        rel_md = md.relative_to(ROOT)
        text = md.read_text(encoding="utf-8")
        text = re.sub(r"```.*?```", "", text, flags=re.S)  # ignore code blocks
        for m in LINK_RE.finditer(text):
            target = m.group(1) or m.group(2)
            if target.startswith("mailto:"):
                continue
            if target.startswith(("http://", "https://")):
                t = re.search(r"github\.com/" + re.escape(REPO_SLUG) + r"/issues/new\?template=([\w.\-]+)", target)
                if t and not (template_dir / t.group(1)).is_file():
                    err(f"{rel_md}: links to issue template '{t.group(1)}', which does not exist")
                continue
            path_part, _, anchor = target.partition("#")
            dest = md if path_part == "" else (md.parent / path_part).resolve()
            if not dest.exists():
                err(f"{rel_md}: broken link -> {target}")
                continue
            if anchor and dest.is_file() and dest.suffix == ".md":
                if anchor.lower() not in anchors_of(dest):
                    err(f"{rel_md}: link {target} -> no heading '#{anchor}' in {dest.relative_to(ROOT)}")


def main():
    check_required()
    check_stls()
    check_links()
    if errors:
        print(f"Repository check FAILED ({len(errors)} problem(s)):")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)
    n_md = sum(1 for _ in markdown_files())
    print(f"Repository check passed: {len(REQUIRED_FILES)} required files present, "
          f"STLs valid, links in {n_md} Markdown files OK.")


if __name__ == "__main__":
    main()
