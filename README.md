# ffmpeg-m

> FFmpeg as C++ modules for mcpp — built entirely from source, API unchanged.

```cpp
import std;
import ffmpeg.av;   // the whole FFmpeg surface, Python `import av` style

int main() {
    AVFormatContext* fmt { nullptr };
    avformat_open_input(&fmt, "video.mp4", nullptr, nullptr);
    // ... the exact same C API you already know — just no #include
}
```

- **Source build, no CMake/configure at build time.** mcpp compiles all of
  FFmpeg n8.1.2 (2124 `.c` + 157 NASM `.asm`, x86 SIMD included) directly —
  cold build ≈ 21 s on 32 cores.
- **API and habits unchanged.** Every public function, type, enum, and enum
  constant is re-exported under its upstream global name (`avcodec_open2`,
  `AVFrame`, `AV_CODEC_ID_H264`, …).
- **Full-feature profile.** All internal components are on: 600+ decoders,
  350+ demuxers, encoders, muxers, filters, swscale/swresample — hermetic
  (no host libraries probed), LGPL-2.1+ clean.

## Modules

| import | contents |
|---|---|
| `ffmpeg.av` | everything below (recommended default) |
| `ffmpeg.avutil` / `ffmpeg.avcodec` / `ffmpeg.avformat` / `ffmpeg.avfilter` / `ffmpeg.avdevice` / `ffmpeg.swscale` / `ffmpeg.swresample` | one library each; dependencies are re-exported (`import ffmpeg.avformat;` brings avcodec + avutil) |
| `ffmpeg` | lib-root alias of `ffmpeg.av` |

## Macros and header-only helpers

Named modules cannot export macros or `static inline` helpers. Everything
with linkage comes from the import; for the macro spellings (`AVERROR`,
`AV_NOPTS_VALUE`, `AVIO_FLAG_WRITE`, `MKTAG`, …) and inline utilities
(`av_q2d`, `av_clip`, …) add the optional side header **before** the import:

```cpp
#include <ffmpeg-m/macros.h>
import ffmpeg.av;
```

C-only compound-literal macros get C++ replacements exported from the
module: `av_err2str` → `av_err2string(err).c_str()`, `av_ts2str` →
`av_ts2string`, `av_ts2timestr` → `av_ts2timestring`, `av_fourcc2str` →
`av_fourcc2string`.

## Using

```toml
[dependencies]
ffmpeg = "0.0.1"
```

Or start from the template: `mcpp new myplayer --template ffmpeg` (decode
skeleton). Examples: [`examples/probe`](examples/probe) (no input needed),
[`examples/decode_frames`](examples/decode_frames).

Validate the package: `mcpp build && mcpp test` (self-contained
encode→decode roundtrip + API-surface tests).

## Layout

```text
third_party/ffmpeg/   FFmpeg n8.1.2, byte-identical vendor (LGPL-2.1+ profile)
gen/<os-arch>/        configure snapshot: config.h, config_components.{h,asm},
                      config.asm, avconfig.h, ffversion.h, *_list.c
src/*.cppm            module layer (GMF include + export using, generated)
src/gen_exports/      generated export lists + skip reports
include/ffmpeg-m/     optional macros side header
tools/                gen_config.sh (configure→snapshot), gen_sources.py
                      (make -n → source list), gen_exports.py (headers → exports)
```

Regenerating after an FFmpeg bump: `CC=cc sh tools/gen_config.sh && python3
tools/gen_exports.py` — the mcpp.toml sources diff is the audit trail.

## Notes

- Hardware-acceleration headers that need external SDKs
  (`libavutil/hwcontext_cuda.h`, VAAPI, …) are not part of the module export
  surface; with the SDK present, include them textually next to the import.
- Optional variants (external codecs, GPL components, slim profiles) are
  planned as mcpp features backed by per-profile config snapshots.
- License: package code under Apache-2.0-style terms is not possible here —
  the package vendors upstream **LGPL-2.1-or-later** sources (no
  `--enable-gpl`/`nonfree` components); source distribution satisfies LGPL
  relinking by construction.
