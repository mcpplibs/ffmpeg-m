// ffmpeg.avdevice — libavdevice re-exported as a C++ module (C API unchanged).
module;

extern "C" {
#include "gen_exports/avdevice.includes.inc"
}

export module ffmpeg.avdevice;

export import ffmpeg.avformat;

#include "gen_exports/avdevice.inc"
