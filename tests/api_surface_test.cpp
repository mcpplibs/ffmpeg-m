// API-surface smoke tests: the imported module layer must expose the C API
// unchanged (names, enums, versions), and the optional macros side header
// must coexist with the import in one TU.
#include <gtest/gtest.h>

#include <cerrno>
#include <string_view>

// Canonical mixing order: textual includes BEFORE the import (the reverse is
// compiler-hostile territory across the ecosystem).
#include <ffmpeg-m/macros.h>

import ffmpeg.av;

TEST(ApiSurface, VersionsMatchVendoredRelease) {
    EXPECT_EQ(avutil_version() >> 16, 60u);      // n8.1.x: libavutil major 60
    EXPECT_EQ(avcodec_version() >> 16, 62u);
    EXPECT_EQ(avformat_version() >> 16, 62u);
}

TEST(ApiSurface, FullDecoderSetIsPresent) {
    EXPECT_NE(avcodec_find_decoder(AV_CODEC_ID_H264), nullptr);
    EXPECT_NE(avcodec_find_decoder(AV_CODEC_ID_HEVC), nullptr);
    EXPECT_NE(avcodec_find_decoder(AV_CODEC_ID_AV1), nullptr);
    EXPECT_NE(avcodec_find_decoder(AV_CODEC_ID_VP9), nullptr);
    EXPECT_NE(avcodec_find_decoder(AV_CODEC_ID_AAC), nullptr);
    EXPECT_NE(avcodec_find_decoder(AV_CODEC_ID_OPUS), nullptr);
    EXPECT_NE(avcodec_find_decoder(AV_CODEC_ID_FLAC), nullptr);
}

TEST(ApiSurface, EnumsAndTypesAreUsable) {
    AVRational half { 1, 2 };
    EXPECT_EQ(half.num, 1);
    AVMediaType type { AVMEDIA_TYPE_VIDEO };
    EXPECT_EQ(type, AVMEDIA_TYPE_VIDEO);
    AVPixelFormat fmt { AV_PIX_FMT_YUV420P };
    EXPECT_EQ(av_get_pix_fmt_name(fmt), std::string_view { "yuv420p" });
}

TEST(ApiSurface, MacroSideHeaderCoexistsWithImport) {
    EXPECT_LT(AVERROR(EAGAIN), 0);                       // function-like macro
    EXPECT_EQ(AV_NOPTS_VALUE, AV_NOPTS);                 // macro vs module constant
    EXPECT_DOUBLE_EQ(av_q2d(AVRational { 1, 4 }), 0.25); // static-inline helper
    EXPECT_EQ(AV_TIME_BASE_Q.num, av_time_base_q().num);
}

TEST(ApiSurface, ErrorStringReplacementWorks) {
    auto text = av_err2string(AVERROR(ENOENT));
    EXPECT_GT(std::string_view { text.c_str() }.size(), 0u);
}
