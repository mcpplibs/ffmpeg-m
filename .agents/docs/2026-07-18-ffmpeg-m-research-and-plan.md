# ffmpeg-m — FFmpeg 源码构建 + C++ 模块封装:调研 + 规划文档

> **状态**: 待 review(决策点见 §5)
> **日期**: 2026-07-18
> **前置**: 本项目由 opencv-m 规划派生(用户指定 FFmpeg 先行);opencv-m 侧文档见
> `github/opencv-m/.agents/docs/2026-07-17-opencv-m-research-and-plan.md` 与同日 `mcpp-feature-requests.md`(G 清单已随 mcpp 0.0.95 全部落地)

**Goal:** 把 FFmpeg(n8.1.2)打包为 mcpp 包 `ffmpeg`:FFmpeg 源码单独 vendor 一个目录,由 **mcpp 直接源码构建**(构建期零 `./configure`/make),含 x86 NASM 汇编;外加 C++ 模块封装层,使用者 `import ffmpeg.av;`(或按库 `import ffmpeg.avcodec;`),**C API 原名原样,不改任何使用习惯**。服务两类用户:独立音视频开发 + opencv-m videoio 的 FFmpeg 后端。

**Architecture:** 与 opencv-m 同构的三件套——`third_party/ffmpeg/`(上游源码原样)+ `gen/<os-arch>/`(configure 一次性快照,纯文本)+ `src/*.cppm`(GMF `extern "C" #include` + `export using` 重导出)。源列表由 `make -n` 机械推导(工具脚本固化)。

**Tech Stack:** mcpp ≥ 0.0.95(汇编一等公民 / per-glob flags / features.sources / build.mcpp 依赖执行——本项目全部用得上)/ nasm 3.02(xlings)/ gcc 16.1.0 / FFmpeg n8.1.2(2026,LGPL-2.1+ 默认)

---

## 1. 可行性验证(F0 POC,已全部跑通,2026-07-18)

**两轮 spike 均端到端通过**(scratchpad `spike-ff/`):

| 轮次 | 配置 | 源文件 | 结果 |
|---|---|---|---|
| ① 纯 C | `--disable-x86asm`,h264+mov+file 最小档 | 223 个 `.c` | `mcpp run` → `avformat=4066406 avcodec=4070502 h264_open=0` ✓ |
| ② x86 汇编 | 同上 + x86asm | 263 源(含 **27 个 `.asm`**) | 运行同 ✓;最终二进制含 **333 个 ssse3/avx2 符号**(如 `avg_h264_chroma_mc8_*_ssse3`),NASM 产物实锤进链 |

消费端代码(验证"习惯不变"):

```cpp
import std;
import ffmpeg.av;
int main() {
    const AVCodec* dec = avcodec_find_decoder_by_name("h264");   // C API 原名
    AVCodecContext* ctx = avcodec_alloc_context3(dec);
    int rc = avcodec_open2(ctx, dec, nullptr);   // rc == 0
    ...
}
```

POC 期间确认的关键事实:

1. **`make -n` 是可靠的源列表推导器**:configure 后 `make -n` 输出全部编译命令,正则抽取 `.c/.S/.asm` 即得精确清单(223/263 个),含生成文件的引用关系。
2. **mcpp 0.0.95 的 `.asm` 一等公民支持工作正常**:`-f elf64` 自动按目标推导;`asmflags` 经 per-glob `flags` 传入(`-I` 与 `-Pconfig.asm`);nasm 从 xlings 解析。
3. **汇编需要两个快照文件**:`config.asm` **和 `config_components.asm`**(后者由 `.asm` 文件自行 `%include`,漏拷会报 `CONFIG_*_DECODER not defined`——第一轮踩坑实录)。
4. `ffversion.h` 由 `ffbuild/version.sh` 在 **make 期**生成(非 configure 期),快照时需以相对路径调用避免生成非法 include guard(POC 踩坑:绝对路径出 `GEN_AVUTIL/FFVERSION_H` 这种带 `/` 的宏名)。
5. FFmpeg TU 编译要素:`c_standard = "c17"`、`-DHAVE_AV_CONFIG_H` + configure 的 CPPFLAGS 组(`-D_ISOC11_SOURCE -D_FILE_OFFSET_BITS=64 ...`)、`-pthread`、链接 `-lpthread -lm`。
6. mcpp 同名基名对象(3 个库各有 `utils.c`/`version.c`)无冲突,263 TU 并行冷构建秒级(32 核)。

## 2. 调研:FFmpeg n8.1.2 源码与构建结构

### 2.1 库结构(7 个库,**libpostproc 已不存在**)

依赖拓扑:`avutil ← {swscale, swresample, avcodec} ← avformat ← avdevice`;avfilter ← avutil(scale 滤镜额外用 swscale)。

| 库 | .c | .asm(NASM) | .S(GAS) | 角色 |
|---|---|---|---|---|
| libavutil | 196 | 12 | 14+ | 基础:mem/error/frame/buffer/pixfmt/opt/dict/tx |
| libavcodec | 1626 | 111 | 176 | 编解码器/parser/BSF/DSP,绝对大头 |
| libavformat | 592 | 0 | 0 | (de)muxer + protocol(avio) |
| libavfilter | 594 | 41 | 5 | 滤镜图(最小档不需要) |
| libswscale | 72 | 11 | 21 | 像素格式转换/缩放 |
| libswresample | 26 | 3 | 4 | 音频重采样 |
| libavdevice | 50 | 0 | 0 | 采集/渲染设备(不需要) |

全树 ≈ 3156 `.c` / 1146 `.h` / **178 `.asm`** / 218 `.S`,约 170 万 LOC。组件宇宙:**607 decoders / 272 encoders / 68 parsers / 366 demuxers / 184 muxers / 54 protocols / 593 filters**——`--disable-everything` 后按需 opt-in。

**最小 "解码 mp4/h264 + 缩放" 档只需 4 库**:avutil + avcodec + avformat + swscale(直接用 swscale API,不必进 avfilter)。

### 2.2 生成文件全集(configure 产出,全部可快照)

| 文件 | 谁需要 | 说明 |
|---|---|---|
| `config.h` / `config_components.h` | 编译 FFmpeg 自身 | 所有 TU 带 `-DHAVE_AV_CONFIG_H`;公共头对 config.h 的引用全部有 `#ifdef HAVE_AV_CONFIG_H` 门 |
| `config.asm` / `config_components.asm` | 编译 `.asm` | `-Pconfig.asm` 预включ + `.asm` 自行 `%include` 后者 |
| `libavutil/avconfig.h` | **消费者也需要**(唯一) | 被 `bswap.h/macros.h/intreadwrite.h/pixfmt.h` 无条件 include;按 arch 有差异(`AV_HAVE_BIGENDIAN`) |
| `libavutil/ffversion.h` | 编译 version.c | make 期由 version.sh 生成,快照即冻结版本串 |
| `*_list.c` ×9(codec/parser/bsf/demuxer/muxer/protocol/filter/indev/outdev) | 编译对应注册 TU | **被 `allcodecs.c` 等文本 `#include`,不单独编译**;由 configure 的 `print_enabled_components()` 生成 |
| `ffbuild/config.mak`、`config.sh`、`.pc`、`.ver` | 仅上游 make/共享库 | mcpp 构建不需要 |

**与 OpenCV 的对应关系工整**:消费者可见的生成头 OpenCV 是 `opencv_modules.hpp`,FFmpeg 是 `avconfig.h`;编译期快照集 OpenCV 是 cvconfig.h+simd_declarations,FFmpeg 是 config.h+config_components+list.c 族。同一套 `gen/` 方法论完全复用。

### 2.3 源选择机制与推导

`lib*/Makefile` 里 `OBJS-$(CONFIG_H264_DECODER) += h264dec.o ...`、`X86ASM-OBJS-$(CONFIG_H264QPEL) += x86/h264_qpel.o`;`ffbuild/arch.mak` 按 `HAVE_X86ASM` 等收编 arch 对象;组件间 `_select` 链传递(h264 → cabac/golomb/h264dsp/...)。**推导方法定为 `make -n` 解析**(configure 后一次,ground truth,工具脚本 `tools/gen_sources.py` 固化),不做 Makefile 静态解析(要复现 `_select` 闭包,费而不稳)。

### 2.4 汇编机制

- x86:NASM 语法 `.asm`,共享基建 `libavutil/x86/{x86inc.asm,x86util.asm}`,上游 flags = `-f elf64 -I<root>/ -I<文件所在目录>/ -Pconfig.asm`(`ffbuild/common.mak:55`)。**注意 NASM 不做文件相对搜索**,`-I` 必须给全(root、各库 `x86/` 目录),目录要带尾斜杠。
- ARM/AArch64/RISC-V:GAS 语法 `.S`(带 C 预处理),由 C 编译器驱动直接汇编——mcpp 0.0.95 的 `.S` 支持覆盖,后续做 aarch64 时启用。
- **纯 C 兜底是上游一等机制**:`HAVE_X86ASM=0` 时 `x86/*_init.c` 根本不编译,DSP 函数指针保持 C 实现——`--disable-x86asm` 即得 100% 纯 C 构建,这是我们跨平台/降级的正确性基线。

### 2.5 对 C++ 模块封装的地形

- **C17**,C11 起步(stdatomic/_Generic/复合字面量);Linux/glibc 下 `COMPAT_OBJS` 为空。
- **公共头大多没有 `extern "C"` 守卫**(仅 8 个头有 `__cplusplus` 块)——封装层 GMF 里必须自套 `extern "C" { #include ... }`(POC 已如此,工作正常)。
- **C-only 复合字面量宏**(C++ 下连传统 `#include` 都不能用,模块层反而是改善机会):`av_err2str`、`av_ts2str`、`av_ts2timestr`、`av_fourcc2str`——封装层导出 inline C++ 等价物(如 `av_err2string() -> std::string`)。`AV_TIME_BASE_Q` 上游已有 `__cplusplus` 分支,安全。
- **C++ 安全的常用宏**:`AVERROR(e)`、`MKTAG`、`AV_VERSION_INT`、`AVERROR_*`、`av_q2d` 等(rational 族是 static inline 函数)→ 常量宏转 `inline constexpr` 导出 + 函数式宏放伴随头 `ffmpeg-m/macros.h`。
- 符号全带 `av_/avcodec_/avformat_/sws_/swr_` 前缀,无命名空间冲突压力 → **全局原名重导出**成立。

### 2.6 许可证(LGPL 合规)

默认 **LGPL v2.1+**(不传 `--enable-gpl/version3/nonfree`);最小解码档(h264/hevc/aac + mov/mkv/mpegts + file + swscale)全部 LGPL 干净。**源码分发形态天然满足 LGPL 静态链接的"可重链接"要求**(消费者本来就从源码构建)。GPL 部件(个别 asm、滤镜)未来若做 feature 必须显式翻转 license 标注,默认永不启用。

## 3. 设计

### 3.1 仓库布局

```text
ffmpeg-m/
├── mcpp.toml                    # 单包: name = "ffmpeg"
├── third_party/ffmpeg/          # n8.1.2 源码, 原封不动 (git subtree)
├── gen/
│   └── linux-x86_64/            # 每 (os,arch) 一份快照
│       ├── config.h  config_components.h  config.asm  config_components.asm
│       ├── libavutil/{avconfig.h, ffversion.h}
│       ├── libavcodec/{codec_list.c, parser_list.c, bsf_list.c}
│       └── libavformat/{demuxer_list.c, muxer_list.c, protocol_list.c}
├── sources.lock/                # tools/gen_sources.py 产物: 每档位每平台的精确源列表
│   └── linux-x86_64.toml        #   (mcpp.toml include 或生成进 [build].sources)
├── src/
│   ├── ffmpeg.cppm              # lib-root: export import ffmpeg.av;
│   ├── av.cppm                  # 汇总模块 ffmpeg.av (= export import 各库模块)
│   ├── avutil.cppm  avcodec.cppm  avformat.cppm  swscale.cppm
│   └── */exports.inc            # 生成 + 人工白名单叠加的导出清单
├── include/ffmpeg-m/macros.h    # AVERROR 等函数式宏伴随头 (可选 include)
├── tools/
│   ├── gen_config.sh            # 打包期一次性: configure → 快照 gen/ (含 ffversion.h 正确生成)
│   └── gen_sources.py           # make -n 解析 → 源列表
├── patches/                     # 目前为空
├── examples/  tests/  docs/  .agents/docs/
```

### 3.2 模块命名(对标 Python 生态的 `import av` / PyAV)

- 按库:`ffmpeg.avutil` / `ffmpeg.avcodec` / `ffmpeg.avformat` / `ffmpeg.swscale`(后续 `ffmpeg.avfilter`、`ffmpeg.swresample`、`ffmpeg.avdevice`)
- 汇总:**`ffmpeg.av`**(`export import` 全部启用库)——消费者默认入口
- lib-root:`src/ffmpeg.cppm`(mcpp 惯例,`export import ffmpeg.av;`)

### 3.3 导出策略:全局原名重导出

```cpp
module;
extern "C" {
#include <libavformat/avformat.h>
}
export module ffmpeg.avformat;
export using ::AVFormatContext;
export using ::avformat_open_input;   // 原名, 零迁移
...
```

C API 无重载/模板/命名空间,`export using` 全兼容。**枚举常量**(`AV_CODEC_ID_*` 等,无链接的 enumerator)是唯一需要验证的角落——P1 里实测 gcc 对 `export using ::AV_CODEC_ID_H264` 的处理,不行则退 `inline constexpr`(imgui-m 已有先例)。导出清单由脚本从公共头枚举生成(头面小:avformat 4 个头、avcodec 25、swscale 3;avutil 99 个头按需白名单),人工段叠加。

### 3.4 mcpp.toml 关键机制(0.0.95 能力全用上)

```toml
[package]
name = "ffmpeg"

[build]
c_standard = "c17"
include_dirs = ["gen/linux-x86_64", "gen/linux-x86_64/libavcodec",
                "gen/linux-x86_64/libavformat", "third_party/ffmpeg"]
cflags = ["-DHAVE_AV_CONFIG_H", "-D_ISOC11_SOURCE", "-D_FILE_OFFSET_BITS=64",
          "-D_LARGEFILE_SOURCE", "-D_POSIX_C_SOURCE=200112", "-D_XOPEN_SOURCE=600",
          "-DPIC", "-fomit-frame-pointer", "-fno-math-errno", "-fno-signed-zeros", "-pthread"]
ldflags = ["-lpthread", "-lm"]
flags = [
  { glob = "third_party/ffmpeg/**", cflags = ["-w"] },     # 三方告警隔离, 封装层保持告警
  { glob = "third_party/ffmpeg/**/*.asm",
    asmflags = ["-Igen/linux-x86_64/", "-Ithird_party/ffmpeg/",
                "-Ithird_party/ffmpeg/libavcodec/x86/", "-Ithird_party/ffmpeg/libavutil/x86/",
                "-Pconfig.asm"] },
]
# sources = 显式清单(sources.lock 生成), 汇编 .asm 仅 x86 目标(cfg 条件 sources 门控)
```

平台矩阵靠 cfg 条件 sources / `[target.'cfg(...)']`(0.0.95)选择 `gen/<os-arch>/` 与对应源列表;v0.1 只做 linux-x86_64。

### 3.5 组件档位与 features

配置快照是**冻结的**——任意组件组合意味着任意 config_components.h,无法用 additive features 表达。因此:

- **v0.1 固定一个档位 `decode-min`**:decoders h264/hevc/aac(+mpeg4 待定)、parsers 同名、demuxers mov/matroska/mpegts、protocol file、swscale。全 LGPL、零外部依赖(`--disable-autodetect --disable-zlib`,pthread 保留)。
- 后续档位(decode-full / +encoders / +network...)= **成套快照 + 成套源列表**,以 feature 选择整档(feature gate sources + include_dirs 无法按 feature 切换 → 档位间 config 差异用 `gen/<profile>-<arch>/` + 档位专属 defines 解决;具体机制 F3 再定,可能需要 mcpp 支持 feature 条件 include_dirs——潜在新 G 项)。
- zlib:v0.1 禁用(mov/mkv 极少数压缩头场景不支持);后续档位经 `compat.zlib` 依赖启用。

### 3.6 与 opencv-m 的集成(F2)

opencv videoio 的 `cap_ffmpeg` 后端需要 avformat/avcodec/swscale(+音频场景 swresample)。opencv-m 依赖 `ffmpeg = { version = "...", visibility = "private" }`,include_dirs 自动传播,opencv 的 `cvconfig.h` 快照翻 `HAVE_FFMPEG`。两包各自独立发版,互不 vendor。

## 4. 风险与未决

- **枚举常量导出**(§3.3)——P1 首个验证项,有成熟退路。
- **decode 正确性验证需要真实样本**:tests 里放一个自生成的微型 mp4(可用上游 make fate 的最小样本或十几 KB 手工样本入库),端到端 decode 首帧断言像素校验和。
- **档位×平台快照矩阵的维护成本**:每格 = configure 一次 + 两个产物目录,`tools/` 全自动化;CI 矩阵逐步铺。
- **上游升级**:pin n8.1.2;升级 = subtree pull + 重跑 gen_config/gen_sources + diff 审查(sources.lock 的 diff 即变更审计面)。
- aarch64 的 `.S` 走 cc 驱动理论无碍但未实测(F3 验证)。

## 5. 待 review 决策点

1. **汇总模块名 `ffmpeg.av`**(对标 PyAV 的 `import av`)+ lib-root `ffmpeg` 转发——OK?
2. **v0.1 档位内容**:h264/hevc/aac 解码 + mov/mkv/mpegts + file + swscale;**要不要 +mpeg4/mjpeg 解码器、+swresample**(opencv 音频路径与更广样本兼容,代价源列表 +几十文件)?
3. **全局原名重导出**(`avformat_open_input` 直接可用,零迁移)vs 命名空间化(`ffmpeg::avformat_open_input`,更"现代"但破坏习惯)——我建议前者,与 opencv-m 的"不改用法"原则一致。
4. **C-only 宏的 C++ 替代物命名**:`av_err2str` → `av_err2string()`(返回 `std::string`)这类命名规则确认。
5. **版本 pin n8.1.2**(当前最新点版本)——OK?
6. **仓库归属**:`mcpplibs/ffmpeg-m` 已建,按 Form-A 模式(仓库内 mcpp.toml,index 只登记 tag)——OK?

## 6. 路线图

| 阶段 | 交付物 | 验证 |
|---|---|---|
| F0 ✅ | 双轮 POC(纯 C + NASM) | 已过(§1) |
| F1a 骨架 | vendor n8.1.2(subtree);`tools/gen_config.sh`+`gen_sources.py`;`gen/linux-x86_64` + `sources.lock` 入库 | `mcpp build` 全源直编绿(=POC 在仓库内复现) |
| F1b 模块层 | `ffmpeg.{avutil,avcodec,avformat,swscale}` + `ffmpeg.av` + 导出清单生成脚本 + `macros.h`;枚举常量导出验证 | `mcpp test`:API 面编译 + 真实 mp4 解码首帧校验和 |
| F1c 收尾 | examples(decode_frames / extract_thumbnail)、README、docs | 新工程 5 行代码解出首帧 |
| F2 opencv 集成 | opencv-m 依赖 ffmpeg 包,videoio `cap_ffmpeg` 启用 | `VideoCapture("x.mp4")` 端到端 |
| F3 扩展 | aarch64(.S)、macOS 快照、档位机制(decode-full)、clang 验证、登记 mcpp-index | CI 矩阵绿 |

---

## 附录 A:F0 POC 复现要点

```bash
# 1) configure (打包期一次, /usr/bin/gcc 即可 — 只为生成配置, 不产二进制)
./configure --disable-everything --disable-programs --disable-doc --disable-autodetect \
  --disable-avdevice --disable-avfilter --disable-swresample --disable-swscale \
  --disable-network \
  --enable-decoder=h264 --enable-parser=h264 --enable-demuxer=mov --enable-protocol=file
# (纯 C 轮加 --disable-x86asm)

# 2) 源列表推导
make -n | grep -oE '[a-zA-Z0-9_/.-]+\.(c|S|asm)\b' | sort -u   # → 223 / 263 项

# 3) 快照: config.h config_components.h config.asm config_components.asm
#    libavutil/{avconfig.h,ffversion.h} libavcodec/{codec,parser,bsf}_list.c
#    libavformat/{demuxer,muxer,protocol}_list.c
#    ffversion.h: cd <builddir> && sh <src>/ffbuild/version.sh <src> libavutil/ffversion.h (相对路径!)
```

POC mcpp.toml 核心段见 §3.4(实测通过);踩坑记录见 §1(config_components.asm、ffversion.h guard、NASM -I 全列)。

## 附录 B:参考

- 本地:opencv-m 两份文档(方法论同源);`mcpp/docs/05`(0.0.95 per-glob flags/asm/features.sources/generated_files)、`docs/07`(build.mcpp 环境契约);FFmpeg 树内 `ffbuild/common.mak:55`(X86ASMFLAGS)、`configure:8515-8838`(生成文件全集)、`LICENSE.md`
- 先例:[allyourcodebase/ffmpeg](https://github.com/andrewrk/ffmpeg)(build.zig 替换构建系统)、[dmorn/ffmpeg.zig](https://github.com/dmorn/ffmpeg.zig)、Rust [ffmpeg-sys-next](https://crates.io/crates/ffmpeg-sys-next) vendored build、[PyAV](https://github.com/PyAV-Org/PyAV)(`import av` 命名参照)
