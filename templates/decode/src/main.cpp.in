// Decode every video frame of the given file — plain FFmpeg C API, no headers.
import std;
import ffmpeg.av;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::println("usage: {} <video-file>", argv[0]);
        return 64;
    }

    AVFormatContext* fmt { nullptr };
    if (avformat_open_input(&fmt, argv[1], nullptr, nullptr) < 0) {
        std::println("cannot open {}", argv[1]);
        return 1;
    }
    avformat_find_stream_info(fmt, nullptr);

    int video { av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0) };
    const AVCodec* codec { avcodec_find_decoder(fmt->streams[video]->codecpar->codec_id) };
    AVCodecContext* dec { avcodec_alloc_context3(codec) };
    avcodec_parameters_to_context(dec, fmt->streams[video]->codecpar);
    avcodec_open2(dec, codec, nullptr);

    AVPacket* packet { av_packet_alloc() };
    AVFrame* frame { av_frame_alloc() };
    long frames { 0 };
    while (av_read_frame(fmt, packet) >= 0) {
        if (packet->stream_index == video && avcodec_send_packet(dec, packet) >= 0)
            while (avcodec_receive_frame(dec, frame) >= 0) ++frames;
        av_packet_unref(packet);
    }
    avcodec_send_packet(dec, nullptr);
    while (avcodec_receive_frame(dec, frame) >= 0) ++frames;

    std::println("{}: {} ({}x{}), {} frames decoded",
                 argv[1], codec->name, dec->width, dec->height, frames);

    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&dec);
    avformat_close_input(&fmt);
}
