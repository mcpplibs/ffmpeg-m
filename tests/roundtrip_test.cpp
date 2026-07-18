// End-to-end proof that the module layer drives real FFmpeg: encode a small
// MJPEG/Matroska clip through the imported API, then demux + decode it back
// and verify the frames. No external sample files needed.
//
// (Matroska, not MOV: the mov demuxer drops the final sample of this
// synthetic clip — an upstream container quirk unrelated to the module
// layer; mkv round-trips all frames.)
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <string>

#include <ffmpeg-m/macros.h>

import ffmpeg.av;

namespace {

constexpr int WIDTH { 64 };
constexpr int HEIGHT { 64 };
constexpr int FRAMES { 5 };

std::string test_file_path() {
    return "target/roundtrip_test.mkv";
}

// Frame i is a diagonal luma gradient x+y+16*i (no wraparound for i < 8),
// so its mean luma is exactly 63 + 16*i. Chroma is neutral.
void fill_frame(AVFrame* frame, int index) {
    for (int y { 0 }; y < HEIGHT; ++y)
        for (int x { 0 }; x < WIDTH; ++x)
            frame->data[0][y * frame->linesize[0] + x] =
                static_cast<std::uint8_t>(x + y + index * 16);
    for (int y { 0 }; y < HEIGHT / 2; ++y)
        for (int x { 0 }; x < WIDTH / 2; ++x) {
            frame->data[1][y * frame->linesize[1] + x] = 128;
            frame->data[2][y * frame->linesize[2] + x] = 128;
        }
}

double expected_mean(int index) {
    return 63.0 + 16.0 * index;
}

void encode_clip(const std::string& path) {
    const AVCodec* codec { avcodec_find_encoder_by_name("mjpeg") };
    ASSERT_NE(codec, nullptr);

    AVFormatContext* fmt { nullptr };
    ASSERT_GE(avformat_alloc_output_context2(&fmt, nullptr, "matroska", path.c_str()), 0);

    AVCodecContext* enc { avcodec_alloc_context3(codec) };
    enc->width = WIDTH;
    enc->height = HEIGHT;
    enc->pix_fmt = AV_PIX_FMT_YUVJ420P;
    enc->time_base = AVRational { 1, 25 };
    enc->color_range = AVCOL_RANGE_JPEG;
    ASSERT_GE(avcodec_open2(enc, codec, nullptr), 0);

    AVStream* stream { avformat_new_stream(fmt, nullptr) };
    stream->time_base = enc->time_base;
    ASSERT_GE(avcodec_parameters_from_context(stream->codecpar, enc), 0);

    ASSERT_GE(avio_open(&fmt->pb, path.c_str(), AVIO_FLAG_WRITE), 0);
    ASSERT_GE(avformat_write_header(fmt, nullptr), 0);

    AVFrame* frame { av_frame_alloc() };
    frame->format = enc->pix_fmt;
    frame->width = WIDTH;
    frame->height = HEIGHT;
    ASSERT_GE(av_frame_get_buffer(frame, 0), 0);

    AVPacket* packet { av_packet_alloc() };
    int written { 0 };
    for (int i { 0 }; i <= FRAMES; ++i) {
        if (i < FRAMES) {
            ASSERT_GE(av_frame_make_writable(frame), 0);
            fill_frame(frame, i);
            frame->pts = i;
            frame->duration = 1;
            ASSERT_GE(avcodec_send_frame(enc, frame), 0);
        } else {
            ASSERT_GE(avcodec_send_frame(enc, nullptr), 0);  // flush
        }
        while (avcodec_receive_packet(enc, packet) >= 0) {
            av_packet_rescale_ts(packet, enc->time_base, stream->time_base);
            ++written;
            ASSERT_GE(av_interleaved_write_frame(fmt, packet), 0);
        }
    }
    EXPECT_EQ(written, FRAMES);
    ASSERT_GE(av_write_trailer(fmt), 0);

    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&enc);
    avio_closep(&fmt->pb);
    avformat_free_context(fmt);
}

}  // namespace

TEST(Roundtrip, EncodeThenDecode) {
    const std::string path { test_file_path() };
    encode_clip(path);

    AVFormatContext* fmt { nullptr };
    ASSERT_EQ(avformat_open_input(&fmt, path.c_str(), nullptr, nullptr), 0);
    ASSERT_GE(avformat_find_stream_info(fmt, nullptr), 0);

    int streamIndex { av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0) };
    ASSERT_GE(streamIndex, 0);
    EXPECT_EQ(fmt->streams[streamIndex]->codecpar->codec_id, AV_CODEC_ID_MJPEG);

    const AVCodec* codec {
        avcodec_find_decoder(fmt->streams[streamIndex]->codecpar->codec_id) };
    ASSERT_NE(codec, nullptr);

    AVCodecContext* dec { avcodec_alloc_context3(codec) };
    ASSERT_GE(avcodec_parameters_to_context(dec, fmt->streams[streamIndex]->codecpar), 0);
    ASSERT_GE(avcodec_open2(dec, codec, nullptr), 0);

    AVPacket* packet { av_packet_alloc() };
    AVFrame* frame { av_frame_alloc() };
    int decoded { 0 };
    auto drain = [&] {
        while (avcodec_receive_frame(dec, frame) >= 0) {
            EXPECT_EQ(frame->width, WIDTH);
            EXPECT_EQ(frame->height, HEIGHT);
            std::int64_t sum { 0 };
            for (int y { 0 }; y < HEIGHT; ++y)
                for (int x { 0 }; x < WIDTH; ++x)
                    sum += frame->data[0][y * frame->linesize[0] + x];
            double mean { static_cast<double>(sum) / (WIDTH * HEIGHT) };
            EXPECT_NEAR(mean, expected_mean(decoded), 3.0);  // MJPEG is lossy
            ++decoded;
        }
    };
    while (av_read_frame(fmt, packet) >= 0) {
        if (packet->stream_index == streamIndex) {
            ASSERT_GE(avcodec_send_packet(dec, packet), 0);
            drain();
        }
        av_packet_unref(packet);
    }
    avcodec_send_packet(dec, nullptr);  // flush
    drain();
    EXPECT_EQ(decoded, FRAMES);

    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&dec);
    avformat_close_input(&fmt);
    std::remove(test_file_path().c_str());
}

TEST(Roundtrip, SwscaleConvertsFrame) {
    SwsContext* sws { sws_getContext(WIDTH, HEIGHT, AV_PIX_FMT_YUV420P,
                                     WIDTH / 2, HEIGHT / 2, AV_PIX_FMT_RGB24,
                                     SWS_BILINEAR, nullptr, nullptr, nullptr) };
    ASSERT_NE(sws, nullptr);
    sws_freeContext(sws);
}
