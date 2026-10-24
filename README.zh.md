# ffmpeg

> FFmpeg 8.1.2 的 C++23 模块封装，保留 C API。

[English](README.md) · **简体中文** · 上游：[FFmpeg/FFmpeg](https://github.com/FFmpeg/FFmpeg) · [Releases](https://github.com/mcpplibs/ffmpeg-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![CI](https://github.com/mcpplibs/ffmpeg-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/ffmpeg-m/actions/workflows/ci.yml)

基于 `compat.ffmpeg` 的薄模块层；源码、冻结配置与 NASM 支持由供应者提供。

## 快速开始

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

## 模块一览

根模块与 `.av` 聚合 `.avutil`、`.avcodec`、`.avformat`、`.avfilter`、`.avdevice`、`.swscale`、`.swresample`。
`.macros` 映射常用常量；预处理宏与 inline 工具在 import 前包含 `<mwrap/ffmpeg/macros.hpp>`。

## 示例

默认模板编码并解码生成的图像帧，无需外部文件。
[组件信息](examples/probe) · [视频解码](examples/decode_frames) · [详细说明](docs/usage.md)

## 工具链与运行时

支持 Linux、macOS、Windows；使用 `mcpp build`、`mcpp test`。外部硬件 API 仍需要对应头文件与 SDK。
当前源码 profile 未启用 GPL/nonfree 组件。

## 许可证

封装：[MIT](LICENSE)。FFmpeg 供应者：LGPL-2.1-or-later；保留选中源码与许可证通知。
