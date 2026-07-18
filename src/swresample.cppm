// ffmpeg.swresample — libswresample re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/swresample.includes.inc"
}

export module ffmpeg.swresample;

export import ffmpeg.avutil;

#include "gen_exports/swresample.inc"
