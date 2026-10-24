# ffmpeg

> FFmpeg 8.1.2 as C++23 modules, preserving the C API.

**English** · [简体中文](README.zh.md) · [Upstream](https://github.com/FFmpeg/FFmpeg) · [Releases](https://github.com/mcpplibs/ffmpeg-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![CI](https://github.com/mcpplibs/ffmpeg-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/ffmpeg-m/actions/workflows/ci.yml)

Thin module interfaces over `compat.ffmpeg`; sources, frozen configuration and NASM support belong to that provider.

## Modules

- `import mcpplibs.mwrap.ffmpeg;`: Complete FFmpeg module aggregate
- `import mcpplibs.mwrap.ffmpeg.avcodec;`: Audio/video codecs
- `import mcpplibs.mwrap.ffmpeg.avformat;`: Containers and stream I/O
- `import mcpplibs.mwrap.ffmpeg.avutil;`: Common media types and utilities
- `import mcpplibs.mwrap.ffmpeg.macros;`: Common macro constant mappings

> More Modules API -> [Usage and API](docs/usage.md)

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

## Examples

The default template encodes and decodes a generated frame without external files.
[Probe](examples/probe) · [Decode frames](examples/decode_frames) · [Details](docs/usage.md)

## Build and runtime

Linux, macOS and Windows; use `mcpp build` and `mcpp test`. External hardware SDK APIs require separate headers/SDKs.
The source profile does not enable GPL/nonfree components.

## License

Wrapper: [MIT](LICENSE). FFmpeg provider: LGPL-2.1-or-later; preserve selected source and license notices.
