// ffmpeg.swscale — libswscale re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/swscale.includes.inc"
}

export module ffmpeg.swscale;

export import ffmpeg.avutil;

#include "gen_exports/swscale.inc"
