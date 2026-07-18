// Show what the vendored FFmpeg exposes — versions and a few codecs/formats.
import std;
import ffmpeg.av;

int main() {
    std::println("libavutil   {}.{}", avutil_version() >> 16, (avutil_version() >> 8) & 0xff);
    std::println("libavcodec  {}.{}", avcodec_version() >> 16, (avcodec_version() >> 8) & 0xff);
    std::println("libavformat {}.{}", avformat_version() >> 16, (avformat_version() >> 8) & 0xff);

    for (auto name : { "h264", "hevc", "av1", "vp9", "aac", "opus", "flac" }) {
        const AVCodec* dec { avcodec_find_decoder_by_name(name) };
        std::println("decoder {:8} {}", name, dec ? "ok" : "MISSING");
    }

    void* it { nullptr };
    long demuxers { 0 };
    while (av_demuxer_iterate(&it)) ++demuxers;
    std::println("{} demuxers registered", demuxers);
}
