# ffmpeg

> FFmpeg 的 C++ 模块化封装 — `import ffmpeg.av;` 即用 · 三平台 · 全从源码构建,API 不变

[![Release](https://img.shields.io/github/v/release/mcpplibs/ffmpeg-m)](https://github.com/mcpplibs/ffmpeg-m/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Module](https://img.shields.io/badge/module-ok-green.svg)](https://en.cppreference.com/w/cpp/language/modules)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

| [mcpp 构建工具](https://github.com/mcpp-community/mcpp) · [包索引 mcpp-index](https://github.com/mcpp-community/mcpp-index) · [FFmpeg 上游](https://github.com/FFmpeg/FFmpeg) · [Issues](https://github.com/mcpplibs/ffmpeg-m/issues) · [Releases](https://github.com/mcpplibs/ffmpeg-m/releases) |
|:---:|
| [![CI](https://github.com/mcpplibs/ffmpeg-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/ffmpeg-m/actions/workflows/ci.yml) |

## 核心特性

- **纯模块导入** — `import ffmpeg.av;`,消费者代码零 `#include`,用的仍是你熟悉的 C API 与全局名(`avcodec_open2`、`AVFrame`、`AV_CODEC_ID_H264` …)
- **从源码构建,消费端不跑 configure** — [`compat.ffmpeg`](https://github.com/mcpp-community/mcpp-index) 携带冻结 configure 快照 + 精确源码清单,mcpp 直接编译整套 FFmpeg 8.1.2(2124 `.c` + 157 NASM `.asm`,x86 SIMD 含)—— 32 核冷构建 ≈ 21 s;本仓库只是薄薄的模块层
- **全功能 profile** — 600+ decoders / 350+ demuxers / encoders / muxers / filters / swscale / swresample 全开,hermetic(不探测宿主库),LGPL-2.1+ 干净(无 `--enable-gpl`/`nonfree`)
- **三平台** — Linux / macOS / Windows 三平台 CI

## 快速开始

```bash
mcpp new myplayer --template ffmpeg && cd myplayer && mcpp run -- video.mp4    # 解码骨架
```

或在已有项目中手动接入:

```toml
[dependencies]
ffmpeg = "0.0.3"
```

```cpp
import std;
import ffmpeg.av;   // 整套 FFmpeg 表面,Python `import av` 风格

int main() {
    AVFormatContext* fmt { nullptr };
    avformat_open_input(&fmt, "video.mp4", nullptr, nullptr);
    // … 还是你熟悉的 C API,只是不用 #include
    avformat_close_input(&fmt);
    return 0;
}
```

## 模块一览

| import | 内容 |
|---|---|
| `ffmpeg.av` | 下面全部(推荐默认) |
| `ffmpeg.avutil` / `ffmpeg.avcodec` / `ffmpeg.avformat` / `ffmpeg.avfilter` / `ffmpeg.avdevice` / `ffmpeg.swscale` / `ffmpeg.swresample` | 每库一个;依赖自动 re-export(`import ffmpeg.avformat;` 带上 avcodec + avutil) |
| `ffmpeg` | `ffmpeg.av` 的 lib-root 别名 |

命名模块不能导出宏或 `static inline` 帮助函数。有链接的都从 import 来;宏拼写(`AVERROR`、`AV_NOPTS_VALUE`、`AVIO_FLAG_WRITE`、`MKTAG` …)与 inline 工具(`av_q2d`、`av_clip` …)在 import 前加可选侧头:

```cpp
#include <ffmpeg-m/macros.h>
import ffmpeg.av;
```

C 的复合字面量宏有 C++ 替换从模块导出:`av_err2str` → `av_err2string(err).c_str()`、`av_ts2str` → `av_ts2string`、`av_ts2timestr` → `av_ts2timestring`、`av_fourcc2str` → `av_fourcc2string`。

## 示例

| 示例 | 内容 |
|---|---|
| [`examples/probe`](examples/probe) | 版本 / 组件信息(无需输入) |
| [`examples/decode_frames`](examples/decode_frames) | 解码一个视频文件的帧 |

```bash
cd examples/decode_frames && mcpp run -- video.mp4
```

`mcpp build && mcpp test` 验证包(自包含 encode→decode 往返 + API 表面测试)。

## 工具链与运行时

包不固定工具链(mcpp 解析环境默认)。上游 FFmpeg 源码**不 vendored**,经 `compat.ffmpeg` 索引包(官方 ffmpeg.org tarball,GLOBAL + CN 镜像,sha256 锁定)到达消费端;描述符(configure 快照 inline 为 `generated_files` + `make -n` 源码清单)在 mcpp-index(`tools/compat-ffmpeg/`)维护。需要外部 SDK 的硬件加速头(`libavutil/hwcontext_cuda.h`、VAAPI …)不在模块导出面内,SDK 存在时在 import 旁 textual include。外部编解码器 / GPL 组件 / slim profile 等变体计划以 mcpp features(按 profile 的配置快照)提供。

> [!NOTE]
> 早期版本,接口可能调整。问题与想法欢迎提 [issue](https://github.com/mcpplibs/ffmpeg-m/issues)。

## License

封装代码 MIT;上游 FFmpeg 经 `compat.ffmpeg` 提供,**LGPL-2.1-or-later**(无 `--enable-gpl`/`nonfree` 组件),源码分发天然满足 LGPL 重链接要求。
