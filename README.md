# ffmpeg-m

FFmpeg **8.1.2** 的 C++23 模块封装，保留 `AVFrame`、`avcodec_open2` 等上游 C API。

## 快速开始

使用 mcpp **2026.10.24.1**。以下为候选版本，索引发布后可直接创建项目。

```sh
mcpp new video --template ffmpeg.ffmpeg@8.1.2.1
cd video
mcpp run
```

默认模板生成一个三帧样例并解码；处理自己的视频可使用 `mcpp run -- video.mp4`。

## 已有项目

```toml
[dependencies]
"ffmpeg.ffmpeg" = "8.1.2.1"
```

```cpp
import std;
import ffmpeg.av;
int main() {
    std::println("FFmpeg {}", av_version_info());
}
```

## 能力边界

实现由 `compat.ffmpeg` 从源码构建，模块层保留原 API。目标平台为 Linux、macOS、Windows。
`ffmpeg.av` 聚合各库，也可独立 import `ffmpeg.avcodec`、`ffmpeg.avformat` 等子模块。

宏与内部链接帮助函数通过 import 前包含 `<ffmpeg-m/macros.h>` 使用。
外部 codec、GPU SDK 及其他构建 profile 的范围见详细文档。

## 文档

[用法与模块](docs/usage.md) · [版本探针](examples/probe) ·
[视频解码](examples/decode_frames) · [MIT 封装许可](LICENSE)

上游 FFmpeg 的许可由对应 `compat.ffmpeg` 构建配置决定，本候选使用 LGPL 配置。
