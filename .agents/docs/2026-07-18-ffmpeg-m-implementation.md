# ffmpeg-m 实现记录(v0.0.1,全功能档)

> 日期: 2026-07-18
> 前置: 同目录 `2026-07-18-ffmpeg-m-research-and-plan.md`(决策点按推荐执行,用户确认)
> 状态: `mcpp build` + `mcpp test`(2 passed)+ 双示例 + 模板 全绿

## 交付形态

- **全功能档一步到位**(用户要求 100% 全功能,替代原 F1 的 decode-min 档):
  configure = `--disable-autodetect --disable-programs --disable-doc`,其余全默认
  → **2281 源(2124 .c + 157 .asm)**,600+ decoders / 272 encoders / 358 demuxers
  / muxers / filters / v4l2·fbdev·oss 设备,LGPL-2.1+,零宿主库依赖。
- **冷构建 ≈ 21s**(32 核,gcc 16.1.0 + nasm 3.02,均由 mcpp/xlings 解析)。
- 模块:`ffmpeg.av`(伞,对标 PyAV `import av`)+ 7 库模块(依赖 `export import`
  镜像库拓扑)+ lib-root `ffmpeg`。**C API 全局原名重导出,用法零迁移**。
- 导出面(生成器统计):函数 978 / struct 194 / typedef 15 / enum 76 /
  枚举常量 1450;`static inline`(内部链接,不可导出)与宏走
  `include/ffmpeg-m/macros.h` 伴随头(include **在 import 之前**)。
- C-only 复合字面量宏的 C++ 替代:`av_err2string` / `av_ts2string` /
  `av_ts2timestring` / `av_fourcc2string`(模块导出,含小型 RAII 缓冲结构)。

## 工具链(三个生成器,升级 FFmpeg 只需重跑)

1. `tools/gen_config.sh` — 打包期 configure(out-of-tree)→ 快照
   `gen/linux-x86_64/`:config.h、config_components.{h,asm}、config.asm、
   avconfig.h、ffversion.h(相对路径调用 version.sh,避免非法 guard)、
   9 个 `*_list.c`(被 allcodecs.c 等文本 include)。
2. `tools/gen_sources.py` — 解析 `make -n` 输出(ground truth)重写 mcpp.toml
   标记区间的 sources;diff 即升级审计面。
3. `tools/gen_exports.py` — 解析各库 Makefile `HEADERS` 块所列安装头 →
   `src/gen_exports/<lib>.{inc,includes.inc,skipped.txt}`。分类:函数(排
   static)/struct·union tag/typedef/enum tag/枚举常量 → `export using`;
   排除:宏名冲突实体(如 av_alloc_size)、SDK 门控头(hwcontext_*、vdpau
   等,FFmpeg **无条件安装**它们但 hermetic 档编不了)。

## mcpp.toml 关键机制(0.0.95 新能力全部用上)

- `.asm` 一等公民:157 个 NASM 文件直接进 sources;per-glob `asmflags`
  提供 `-I`(全列、带尾斜杠;NASM 无文件相对搜索)+ `-Pconfig.asm`。
- per-glob flags:三方 `-w` 告警隔离;子目录库(libavcodec/{aac,bsf,hevc,
  opus,vvc},上游 8.x 新结构)加 `-I<库根>`;每库 `BUILDING_<lib>` define
  (对应 ffbuild/library.mak)。
- `include/` 显式入 include_dirs(伴随头 `<ffmpeg-m/macros.h>`)。

## 实现期踩坑(均已解决,后来者注意)

1. mcpp.toml 的 flags 内联表**不能跨行**(严格 TOML,`[[build.flags]]` 数组表
   报 not supported → 已提 [mcpp#227](https://github.com/mcpp-community/mcpp/issues/227));
   glob **不支持 `{a,b}` 花括号**(会警告 matched-no-source——0.0.95 的防呆
   设计帮了忙 → 已提 [mcpp#228](https://github.com/mcpp-community/mcpp/issues/228))。
2. 模块 TU(C++)编 FFmpeg 头需要 `-D__STDC_CONSTANT_MACROS`(上游 CXXFLAGS
   同款),否则 common.h 直接 #error。
3. 子目录源的引号 include:**用上游同款 per-lib `-I`**;`-iquote` 在 mcpp
   0.0.95 不做项目根相对重写(只有 `-I` 会被绝对化)——避免用
   (→ 已提 bug [mcpp#226](https://github.com/mcpp-community/mcpp/issues/226))。
4. gcc≥14 `implicit-function-declaration` 默认硬错:`-I<libavutil>` 全局加
   会让 `#include <time.h>` 角括号劫持到 libavutil/time.h(nanosleep 消失),
   所以 per-lib `-I` 只能按上游范围给子目录库,不能全局。
5. 生成器 bug 教训:带函数指针**字段**的 struct typedef(AVCodecContext、
   AVInputFormat)曾被误判为函数指针 typedef 而漏导出——fp 分支必须限定
   `not had_body`。
6. 消费 TU 的 include 顺序:**macros.h 必须在 import 之前**(include-after-
   import 在 gcc16 实测触发 av_make_q 不可见;与调研结论一致)。
7. MOV 容器 quirk:合成 5 帧 MJPEG clip,mov demuxer 只返回 4 包(mkv 正常
   5/5)——上游容器行为,与模块层无关;roundtrip 测试用 matroska。

### 复验澄清(2026-07-18,mcpp 0.0.95)

提 issue 前逐条复验,以下两项在 0.0.95 上**已不复现**(0.0.93 时代的记录不再成立,勿再引用):

- `[build].cxxflags` 相对 `-I` 对 `.cppm` 模块 TU:现已生效;
- sources glob 经目录软链接(含 `**`、链接到项目外):现已跟随。

确认为 mcpp 侧问题并已提交:[#226](https://github.com/mcpp-community/mcpp/issues/226)(bug:`-iquote` 相对路径不重写)、[#227](https://github.com/mcpp-community/mcpp/issues/227)(增强:`[[build.flags]]` 数组表)、[#228](https://github.com/mcpp-community/mcpp/issues/228)(增强:glob 花括号交替 / glob 数组)。其余踩坑属 FFmpeg 上游(MOV quirk、`__STDC_CONSTANT_MACROS`)、编译器生态(include-before-import)或本仓生成器自身(函数指针字段误判),不涉及 mcpp。

## 验证

- `mcpp test`:`api_surface_test`(版本号、全解码器集 h264/hevc/av1/vp9/aac/
  opus/flac、enum/类型可用性、宏头共存、av_err2string)+ `roundtrip_test`
  (encode MJPEG→mkv→decode,逐帧亮度均值断言 63+16i±3)。
- `examples/probe`(path-dep 消费者):输出 avutil 60.26 / avcodec 62.28 /
  avformat 62.12,358 demuxers,全解码器 ok。
- `examples/decode_frames` + `templates/decode`(imgui-m 式 template.toml +
  mcpp.toml.in + src/main.cpp.in)。

## 后续(不阻塞发版)

- 平台矩阵:aarch64(`.S` 走 cc 驱动,gen/<target> 双快照 + cfg 条件
  sources)、macOS、musl。
- features:按档位(slim/decode-only、GPL 部件显式翻转 license、外部编解码
  器 compat.* 依赖)= 每档一套 gen/ 快照 + 源列表;等 mcpp feature 条件
  include_dirs 或用 defines 方案定案。
- opencv-m F2 集成:videoio `cap_ffmpeg` 依赖本包(HAVE_FFMPEG 快照翻转)。
- mcpp-index 登记(Form-A,tag tarball)。
