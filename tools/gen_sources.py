#!/usr/bin/env python3
"""gen_sources.py — derive the compiled-source list from `make -n` output.

FFmpeg's Makefiles resolve CONFIG_*/HAVE_* gating and _select closures; make's
dry run is therefore the ground truth for which .c/.S/.asm files a frozen
configure result compiles. The list is target-specific (x86 builds compile
x86/*.c + NASM .asm, aarch64 builds compile aarch64/*.S), so it is written as
a `[target.'cfg(...)'.build]` block — sources plus the private -I flags that
point at that target's gen/ snapshot — between per-target markers in
mcpp.toml. The manifest stays a committed, reviewable artifact: its diff is
the audit trail when bumping FFmpeg or the profile.

The PRIMARY target (linux-x86_64) is written into the [build].sources marker
block instead: dependency builds currently ignore a dependency's
[target.'cfg(...)'.build].sources (mcpp#229), so the must-work platform's list
stays in the unconditional base until that is fixed.

Usage: gen_sources.py <make-n.log> <mcpp.toml> <target>
"""

import re
import sys

TARGET_CFG = {
    "linux-x86_64": 'cfg(all(linux, arch = "x86_64"))',
    "linux-aarch64": 'cfg(all(linux, arch = "aarch64"))',
    "macos-aarch64": 'cfg(all(macos, arch = "aarch64"))',
    "macos-x86_64": 'cfg(all(macos, arch = "x86_64"))',
}

SRC_RE = re.compile(r"(?:^|\s)(src/[A-Za-z0-9_/.+-]+\.(?:c|S|asm))(?:\s|$)")

# Directories holding generated *_list.c files that library sources
# quote-include; each needs a private -I on that target's compile.
LIST_DIRS = ["", "/libavcodec", "/libavformat", "/libavfilter", "/libavdevice"]


def main() -> None:
    log_path, toml_path, target = sys.argv[1], sys.argv[2], sys.argv[3]
    if target not in TARGET_CFG:
        sys.exit(f"gen_sources: unknown target {target!r} "
                 f"(known: {', '.join(TARGET_CFG)})")

    sources: set[str] = set()
    with open(log_path) as log:
        for line in log:
            for match in SRC_RE.finditer(line):
                sources.add(match.group(1).replace("src/", "third_party/ffmpeg/", 1))
    if not sources:
        sys.exit("gen_sources: no sources extracted — is the make -n log empty?")

    ordered = sorted(sources)
    n_asm = sum(1 for s in ordered if s.endswith(".asm"))
    n_gas = sum(1 for s in ordered if s.endswith(".S"))

    if target == "linux-x86_64":
        # Primary platform: unconditional base sources (see module docstring).
        begin = "  # BEGIN GENERATED SOURCES linux-x86_64 (tools/gen_config.sh — do not edit by hand)"
        end = "  # END GENERATED SOURCES linux-x86_64"
        block = "\n".join([begin] + [f'  "{s}",' for s in ordered] + [end])
    else:
        begin = f"# BEGIN GENERATED TARGET BLOCK {target} (tools/gen_config.sh — do not edit by hand)"
        end = f"# END GENERATED TARGET BLOCK {target}"
        cflags = [f"-Igen/{target}{d}" for d in LIST_DIRS]
        block_lines = [
            begin,
            "# NOTE: consumer (dependency) builds need mcpp#229 before this block",
            "# takes effect outside root builds.",
            f"[target.'{TARGET_CFG[target]}'.build]",
            "cflags = [" + ", ".join(f'"{f}"' for f in cflags) + "]",
            f'cxxflags = ["-Igen/{target}"]',
            "sources = [",
            *[f'  "{s}",' for s in ordered],
            "]",
            end,
        ]
        block = "\n".join(block_lines)

    with open(toml_path) as fh:
        manifest = fh.read()

    begin_at, end_at = manifest.find(begin), manifest.find(end)
    if begin_at != -1 and end_at != -1:
        manifest = manifest[:begin_at] + block + manifest[end_at + len(end):]
    elif target == "linux-x86_64":
        sys.exit("gen_sources: base markers for linux-x86_64 not found in mcpp.toml")
    else:
        manifest = manifest.rstrip("\n") + "\n\n" + block + "\n"

    with open(toml_path, "w") as fh:
        fh.write(manifest)

    print(f"gen_sources[{target}]: {len(ordered)} sources "
          f"({len(ordered) - n_asm - n_gas} .c, {n_asm} .asm, {n_gas} .S)")


if __name__ == "__main__":
    main()
