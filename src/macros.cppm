module;
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
namespace mwrap_ffmpeg_capture {
inline constexpr auto value_AV_TIME_BASE = AV_TIME_BASE;
inline constexpr auto value_AV_NOPTS_VALUE = AV_NOPTS_VALUE;
inline constexpr auto value_AVERROR_EOF = AVERROR_EOF;
inline constexpr auto value_AVERROR_INVALIDDATA = AVERROR_INVALIDDATA;
inline constexpr auto value_AV_INPUT_BUFFER_PADDING_SIZE = AV_INPUT_BUFFER_PADDING_SIZE;
inline constexpr auto value_AV_PKT_FLAG_KEY = AV_PKT_FLAG_KEY;
inline constexpr auto value_AVIO_FLAG_READ = AVIO_FLAG_READ;
inline constexpr auto value_AVIO_FLAG_WRITE = AVIO_FLAG_WRITE;
inline constexpr auto value_AVSEEK_SIZE = AVSEEK_SIZE;
}
#undef AV_TIME_BASE
#undef AV_NOPTS_VALUE
#undef AVERROR_EOF
#undef AVERROR_INVALIDDATA
#undef AV_INPUT_BUFFER_PADDING_SIZE
#undef AV_PKT_FLAG_KEY
#undef AVIO_FLAG_READ
#undef AVIO_FLAG_WRITE
#undef AVSEEK_SIZE

export module mcpplibs.mwrap.ffmpeg.macros;

export namespace mcpplibs::mwrap::ffmpeg::macros {
inline constexpr auto AV_TIME_BASE = mwrap_ffmpeg_capture::value_AV_TIME_BASE;
inline constexpr auto AV_NOPTS_VALUE = mwrap_ffmpeg_capture::value_AV_NOPTS_VALUE;
inline constexpr auto AVERROR_EOF = mwrap_ffmpeg_capture::value_AVERROR_EOF;
inline constexpr auto AVERROR_INVALIDDATA = mwrap_ffmpeg_capture::value_AVERROR_INVALIDDATA;
inline constexpr auto AV_INPUT_BUFFER_PADDING_SIZE = mwrap_ffmpeg_capture::value_AV_INPUT_BUFFER_PADDING_SIZE;
inline constexpr auto AV_PKT_FLAG_KEY = mwrap_ffmpeg_capture::value_AV_PKT_FLAG_KEY;
inline constexpr auto AVIO_FLAG_READ = mwrap_ffmpeg_capture::value_AVIO_FLAG_READ;
inline constexpr auto AVIO_FLAG_WRITE = mwrap_ffmpeg_capture::value_AVIO_FLAG_WRITE;
inline constexpr auto AVSEEK_SIZE = mwrap_ffmpeg_capture::value_AVSEEK_SIZE;

}
