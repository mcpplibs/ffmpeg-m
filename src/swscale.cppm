// ffmpeg.swscale — libswscale re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/swscale.includes.inc"
}

export module mcpplibs.mwrap.ffmpeg.swscale;

export import mcpplibs.mwrap.ffmpeg.avutil;

#include "gen_exports/swscale.inc"
