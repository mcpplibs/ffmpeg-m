// ffmpeg.avformat — libavformat re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/avformat.includes.inc"
}

export module ffmpeg.avformat;

export import ffmpeg.avcodec;

#include "gen_exports/avformat.inc"
