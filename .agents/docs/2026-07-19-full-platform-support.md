# ffmpeg-m — full-platform (macOS + Windows) support

This repo's module layer (`import ffmpeg.av;` + per-lib modules) is
platform-neutral. Full-platform support is gated on **`compat.ffmpeg` gaining
macOS/Windows config snapshots** (a mcpp-index concern).

**Authoritative plan:** `mcpp-index/.agents/docs/2026-07-19-full-platform-support-plan.md`.

Key ffmpeg-specific points:
- macOS-arm64: `./configure --cc=<mcpp-clang> --disable-autodetect` works; SIMD
  becomes aarch64 GAS `.S` (no NASM) — mcpp `.S`-on-macOS is the #1 de-risk (R2).
  Drop `config.asm`/`config_components.asm` (NASM-only) for non-x86.
- Windows-x86_64: **highest risk** — configure needs MSYS2/bash, MSVC-ABI target,
  and clang-MSVC must compile ~2100 FFmpeg C TUs (no precedent). Spike-gated.
- The compat descriptor + generators live in mcpp-index `tools/compat-ffmpeg/`
  (moved there); this repo only widens `platforms` + CI as platforms land.
Sequencing: compat.ffmpeg-macOS is the FIRST real target (also unblocks
opencv videoio on macOS).
