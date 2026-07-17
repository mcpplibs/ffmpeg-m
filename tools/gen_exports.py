#!/usr/bin/env python3
"""gen_exports.py — generate `export using ::name;` lists from FFmpeg headers.

For each library, the INSTALLED public headers (the `HEADERS` block in the
lib's Makefile) are scanned and every re-exportable name is emitted into
src/gen_exports/<lib>.inc, which the matching src/<lib>.cppm textually
includes inside its module purview. Categories:

  exported   — non-inline API functions, struct/union tags, typedefs, enum
               tags, enumerators (all have linkage / are types → valid
               targets for an exported using-declaration per
               [module.interface]/5; enumerator re-export verified on GCC 16)
  skipped    — `static inline` helpers (internal linkage — not exportable;
               reachable via include/ffmpeg-m/macros.h textual include) and
               object-like/function-like macros (never exportable)

The skip list is written to src/gen_exports/<lib>.skipped.txt so the macros.h
curation stays reviewable.

Usage: gen_exports.py [ffmpeg-root]      (default: repo third_party/ffmpeg)
"""

import re
import sys
from pathlib import Path

LIBS = {
    "avutil": "libavutil",
    "avcodec": "libavcodec",
    "avformat": "libavformat",
    "avfilter": "libavfilter",
    "avdevice": "libavdevice",
    "swscale": "libswscale",
    "swresample": "libswresample",
}

# Public-symbol families; anything else in a header is infrastructure.
NAME_RE = re.compile(r"^(?:av|sws|swr)[a-z0-9_]*$", re.IGNORECASE)
IDENT = r"[A-Za-z_][A-Za-z0-9_]*"

# Installed headers that require external SDK headers (CUDA, VAAPI, D3D, …).
# FFmpeg installs them unconditionally, but they cannot be compiled in the
# hermetic profile and their APIs are unusable without the SDK anyway.
# Consumers with the SDK can textually #include them next to the import.
EXCLUDED_HEADER = re.compile(
    r"^hwcontext_(?!$).+\.h$|^(vdpau|qsv|videotoolbox|dxva2|d3d11va|d3d12va"
    r"|mediacodec|jni|xvmc|vulkan|vulkan_functions|vorbis_parser)\.h$")


def installed_headers(lib_dir: Path) -> list[Path]:
    text = (lib_dir / "Makefile").read_text()
    match = re.search(r"^HEADERS\s*=((?:[^\n]*\\\n)*[^\n]*)", text, re.M)
    if not match:
        return []
    names = match.group(1).replace("\\\n", " ").split()
    return [lib_dir / n for n in names
            if (lib_dir / n).exists() and not EXCLUDED_HEADER.match(n)]


def strip_comments(src: str) -> str:
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    return re.sub(r"//[^\n]*", " ", src)


def strip_preprocessor(src: str) -> tuple[str, list[str]]:
    """Remove preprocessor lines; return (code, macro names defined)."""
    macros: list[str] = []
    kept: list[str] = []
    for line in src.replace("\\\n", " ").split("\n"):
        stripped = line.lstrip()
        if stripped.startswith("#"):
            m = re.match(r"#\s*define\s+(" + IDENT + ")", stripped)
            if m:
                macros.append(m.group(1))
        else:
            kept.append(line)
    return "\n".join(kept), macros


def top_level_statements(code: str):
    """Yield (statement, had_body) at brace depth 0."""
    depth = 0
    buf: list[str] = []
    had_body = False
    for ch in code:
        buf.append(ch)
        if ch == "{":
            depth += 1
            had_body = True
        elif ch == "}":
            depth -= 1
        elif ch == ";" and depth == 0:
            yield "".join(buf).strip(), had_body
            buf, had_body = [], False


def parse_header(path: Path):
    exported: dict[str, str] = {}     # name -> kind
    skipped: list[str] = []

    raw = strip_comments(path.read_text(errors="replace"))
    code, macros = strip_preprocessor(raw)
    skipped += [f"macro {m}" for m in macros]

    for stmt, had_body in top_level_statements(code):
        flat = " ".join(stmt.split())
        if not flat or flat.startswith("extern \"C\""):
            continue

        # -- enums: tag + enumerators ------------------------------------
        m = re.match(r"(?:typedef\s+)?enum\s+(" + IDENT + r")?\s*{(.*)}\s*(" + IDENT + r")?\s*;$",
                     flat, re.S)
        if m:
            tag, body, alias = m.group(1), m.group(2), m.group(3)
            for name in (tag, alias):
                if name:
                    exported[name] = "enum"
            for entry in body.split(","):
                em = re.match(r"\s*(" + IDENT + ")", entry)
                if em and not em.group(1).startswith("__"):
                    exported[em.group(1)] = "enumerator"
            continue

        # -- typedefs ----------------------------------------------------
        if flat.startswith("typedef"):
            # Function-pointer typedef — but only when there is no braced body:
            # a struct typedef whose FIELDS are function pointers must fall
            # through to the alias branch (`} AVCodecContext;`).
            fp = re.search(r"\(\s*\*\s*(" + IDENT + r")\s*\)", flat)
            if fp and not had_body:
                exported[fp.group(1)] = "typedef"
                continue
            m = re.search(r"(" + IDENT + r")\s*(?:\[[^\]]*\]\s*)?;$", flat)
            if m:
                exported[m.group(1)] = "typedef"
            tag = re.match(r"typedef\s+(?:struct|union)\s+(" + IDENT + ")", flat)
            if tag:
                exported[tag.group(1)] = "struct"
            continue

        # -- bare struct/union definitions or forward declarations -------
        m = re.match(r"(?:struct|union)\s+(" + IDENT + ")", flat)
        if m and (had_body or flat.endswith(";")):
            exported[m.group(1)] = "struct"
            continue

        # -- functions ---------------------------------------------------
        if "(" in flat and not had_body:
            m = re.search(r"\b(" + IDENT + r")\s*\(", flat)
            if m:
                name = m.group(1)
                if re.search(r"\bstatic\b", flat.split("(")[0]):
                    skipped.append(f"static-inline {name}")
                elif NAME_RE.match(name):
                    exported[name] = "function"
            continue
        if "(" in flat and had_body:                  # static inline definition
            m = re.search(r"\b(" + IDENT + r")\s*\(", flat.split("{")[0])
            if m:
                skipped.append(f"static-inline {m.group(1)}")

    # Only surface public-family names.
    exported = {n: k for n, k in exported.items()
                if NAME_RE.match(n) or n.startswith(("AV", "Sws", "Swr", "FF"))}
    return exported, skipped


def main() -> None:
    if len(sys.argv) > 1:
        root = Path(sys.argv[1])
    else:
        import subprocess
        fetch = Path(__file__).resolve().parent / "fetch_upstream.sh"
        root = Path(subprocess.check_output(["sh", str(fetch)], text=True).strip())
    out_dir = Path(__file__).resolve().parent.parent / "src" / "gen_exports"
    out_dir.mkdir(parents=True, exist_ok=True)

    parsed = {}          # lib -> (headers, exported, skipped)
    all_macros: set[str] = set()
    for lib, lib_dir_name in LIBS.items():
        lib_dir = root / lib_dir_name
        exported: dict[str, str] = {}
        skipped: list[str] = []
        headers = installed_headers(lib_dir)
        for header in headers:
            exp, skip = parse_header(header)
            exported.update(exp)
            skipped += [f"{header.name}: {s}" for s in skip]
        parsed[lib] = (headers, exported, skipped)
        all_macros |= {s.split()[-1] for s in skipped if " macro " in f" {s} "}

    for lib, lib_dir_name in LIBS.items():
        headers, exported, skipped = parsed[lib]

        # A name that is ALSO an object-/function-like macro anywhere in the
        # public headers cannot appear in an export using-declaration — the
        # macro would expand inside our own module TU. Route those through
        # include/ffmpeg-m/macros.h instead.
        macro_clashes = sorted(set(exported) & all_macros)
        for name in macro_clashes:
            del exported[name]
            skipped.append(f"(macro-clash) {name}")

        # The GMF include set: every installed public header of this library.
        inc_lines = [f"// Auto-generated by tools/gen_exports.py — installed "
                     f"public headers of {lib_dir_name}.", ""]
        inc_lines += [f"#include <{lib_dir_name}/{h.name}>" for h in headers]
        (out_dir / f"{lib}.includes.inc").write_text("\n".join(inc_lines) + "\n")

        lines = [f"// Auto-generated by tools/gen_exports.py from {lib_dir_name}",
                 f"// public headers ({len(headers)} files) — do not edit by hand.",
                 ""]
        for kind in ("struct", "typedef", "enum", "enumerator", "function"):
            names = sorted(n for n, k in exported.items() if k == kind)
            if names:
                lines.append(f"// {kind}s ({len(names)})")
                lines += [f"export using ::{n};" for n in names]
                lines.append("")

        (out_dir / f"{lib}.inc").write_text("\n".join(lines))
        (out_dir / f"{lib}.skipped.txt").write_text("\n".join(sorted(set(skipped))) + "\n")
        counts = {k: sum(1 for v in exported.values() if v == k)
                  for k in ("function", "struct", "typedef", "enum", "enumerator")}
        print(f"{lib}: {counts} skipped={len(set(skipped))}")


if __name__ == "__main__":
    main()
