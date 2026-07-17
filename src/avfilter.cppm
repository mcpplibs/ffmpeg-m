// ffmpeg.avfilter — libavfilter re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/avfilter.includes.inc"
}

export module ffmpeg.avfilter;

export import ffmpeg.avutil;

#include "gen_exports/avfilter.inc"
