// ffmpeg.avdevice — libavdevice re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/avdevice.includes.inc"
}

export module mcpplibs.mwrap.ffmpeg.avdevice;

export import mcpplibs.mwrap.ffmpeg.avformat;

#include "gen_exports/avdevice.inc"
