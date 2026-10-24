# ffmpeg

> FFmpeg 8.1.2 as C++23 modules, preserving the C API.
> Thin module interfaces over `compat.ffmpeg`; sources, frozen configuration and NASM support belong to that provider.

**English** · [简体中文](README.zh.md)

Upstream: [FFmpeg/FFmpeg](https://github.com/FFmpeg/FFmpeg)

[Releases](https://github.com/mcpplibs/ffmpeg-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![CI](https://github.com/mcpplibs/ffmpeg-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/ffmpeg-m/actions/workflows/ci.yml)

## Getting started

```sh
mcpp new media --template mcpplibs.mwrap.ffmpeg@8.1.2
cd media
mcpp run
```

```toml
[dependencies]
mcpplibs.mwrap.ffmpeg = "8.1.2"
```

```cpp
import mcpplibs.mwrap.ffmpeg.av;
int main() { return avcodec_version() == 0; }
```

## Modules

Root and `.av` aggregate `.avutil`, `.avcodec`, `.avformat`, `.avfilter`, `.avdevice`, `.swscale`, `.swresample`.
`.macros` maps common constants. For preprocessing and inline helpers include `<mwrap/ffmpeg/macros.hpp>` before imports.

## Examples

The default template encodes and decodes a generated frame without external files.
[Probe](examples/probe) · [Decode frames](examples/decode_frames) · [Details](docs/usage.md)

## Build and runtime

Linux, macOS and Windows; use `mcpp build` and `mcpp test`. External hardware SDK APIs require separate headers/SDKs.
The source profile does not enable GPL/nonfree components.

## License

Wrapper: [MIT](LICENSE). FFmpeg provider: LGPL-2.1-or-later; preserve selected source and license notices.
