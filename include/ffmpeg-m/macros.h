// ffmpeg-m/macros.h — optional side header for FFmpeg's preprocessor surface.
//
// C++ named modules cannot export macros or static-inline helpers. Everything
// with linkage comes from `import ffmpeg.av;`; consumers who also want the
// macro spellings (AVERROR, MKTAG, AV_NOPTS_VALUE, AV_TIME_BASE_Q, AV_RL32,
// LIBAV*_VERSION_*) or the header-only inline utilities (av_q2d, av_make_q,
// av_clip family, av_rescale_q) textually include THIS header next to the
// import:
//
//     import ffmpeg.av;
//     #include <ffmpeg-m/macros.h>       // optional, macro habits only
//
// The declarations herein denote the same global-module entities the modules
// re-export, so mixing include and import this way is well-formed. Include
// further upstream headers directly (they are on the include path) for
// SDK-gated surfaces such as libavutil/hwcontext_cuda.h.
#ifndef FFMPEG_M_MACROS_H
#define FFMPEG_M_MACROS_H

extern "C" {
#include <libavutil/avutil.h>        // AV_NOPTS_VALUE, AV_TIME_BASE(_Q), av_q2d/av_clip inlines, AVERROR family, MKTAG, versions
#include <libavutil/intreadwrite.h>  // AV_RL32 / AV_WB16 … byte-access macros
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>           // AV_OPT_* flags
#include <libavutil/timestamp.h>     // av_ts_make_string / av_ts_make_time_string (the av_ts2str macro itself stays C-only — use av_ts2string from the module)
#include <libavcodec/avcodec.h>      // AV_INPUT_BUFFER_PADDING_SIZE, AV_PKT_FLAG_*, AV_CODEC_FLAG_*, AV_GET_BUFFER_FLAG_*
#include <libavformat/avformat.h>    // AVIO_FLAG_*, AVSEEK_*, AVFMT_* flags
#include <libavfilter/avfilter.h>    // AVFILTER_FLAG_*
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>      // SWS_* flags
#include <libswresample/swresample.h>
}

#endif // FFMPEG_M_MACROS_H
