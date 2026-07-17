// ffmpeg.avutil — libavutil re-exported as a C++ module.
// The C API is unchanged: every exported name below is the upstream global.
module;

extern "C" {
#include "gen_exports/avutil.includes.inc"
}

export module ffmpeg.avutil;

#include "gen_exports/avutil.inc"

// ---------------------------------------------------------------------------
// C++ replacements for upstream compound-literal macros (av_err2str family).
// Those macros are C-only — they do not compile in C++ even with a textual
// #include — so the module layer is strictly an improvement here. Naming per
// design doc: <macro>2str → <name>2string.
// ---------------------------------------------------------------------------

export struct AvErrorString {
    char text[AV_ERROR_MAX_STRING_SIZE] {};
    const char* c_str() const { return text; }
};

// av_err2str(errnum)
export inline AvErrorString av_err2string(int errnum) {
    AvErrorString s {};
    ::av_strerror(errnum, s.text, sizeof s.text);
    return s;
}

export struct AvTsString {
    char text[AV_TS_MAX_STRING_SIZE] {};
    const char* c_str() const { return text; }
};

// av_ts2str(ts)
export inline AvTsString av_ts2string(int64_t ts) {
    AvTsString s {};
    ::av_ts_make_string(s.text, ts);
    return s;
}

// av_ts2timestr(ts, tb)
export inline AvTsString av_ts2timestring(int64_t ts, AVRational timeBase) {
    AvTsString s {};
    ::av_ts_make_time_string(s.text, ts, &timeBase);
    return s;
}

export struct AvFourccString {
    char text[AV_FOURCC_MAX_STRING_SIZE] {};
    const char* c_str() const { return text; }
};

// av_fourcc2str(fourcc)
export inline AvFourccString av_fourcc2string(uint32_t fourcc) {
    AvFourccString s {};
    ::av_fourcc_make_string(s.text, fourcc);
    return s;
}

// AV_TIME_BASE_Q / AV_NOPTS_VALUE are macros (side header covers the macro
// spelling); constants are also usable directly:
export inline constexpr int64_t AV_NOPTS = INT64_C(0x8000000000000000);
export inline AVRational av_time_base_q() { return ::av_make_q(1, AV_TIME_BASE); }
