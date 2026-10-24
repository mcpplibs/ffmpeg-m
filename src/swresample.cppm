// ffmpeg.swresample — libswresample re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/swresample.includes.inc"
}

export module mcpplibs.mwrap.ffmpeg.swresample;

export import mcpplibs.mwrap.ffmpeg.avutil;

#include "gen_exports/swresample.inc"
