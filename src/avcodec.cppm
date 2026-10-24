// ffmpeg.avcodec — libavcodec re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/avcodec.includes.inc"
}

export module mcpplibs.mwrap.ffmpeg.avcodec;

export import mcpplibs.mwrap.ffmpeg.avutil;

#include "gen_exports/avcodec.inc"
