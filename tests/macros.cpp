import mcpplibs.mwrap.ffmpeg;
namespace macros = mcpplibs::mwrap::ffmpeg::macros;
int main() {
    static_assert(macros::AV_TIME_BASE == 1000000);
    static_assert(macros::AVERROR_EOF < 0);
    static_assert(macros::AVIO_FLAG_READ == 1);
    return avcodec_version() == 0;
}
