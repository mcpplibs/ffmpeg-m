#!/bin/sh
# gen_config.sh — packaging-time snapshot generator for ffmpeg-m.
#
# Runs FFmpeg's ./configure ONCE (out-of-tree) for the given target profile and
# snapshots every generated file mcpp needs into gen/<target>/, then derives the
# exact source list via gen_sources.py. Consumers never run this: the outputs
# are committed. Re-run only when bumping the FFmpeg version or the profile.
#
# Usage: tools/gen_config.sh [target]        (default: autodetected <os>-<arch>)
set -eu

target="${1:-$(uname -s | tr 'A-Z' 'a-z' | sed s/darwin/macos/)-$(uname -m | sed s/arm64/aarch64/)}"
root="$(cd "$(dirname "$0")/.." && pwd)"
src="$root/third_party/ffmpeg"
gen="$root/gen/$target"
bld="$(mktemp -d /tmp/ffmpeg-m-cfg.XXXXXX)"
trap 'rm -rf "$bld"' EXIT

# Full-feature hermetic profile: every INTERNAL component on; nothing probed
# from the host (--disable-autodetect) so the snapshot is reproducible and the
# consumer build needs no system libraries. programs/doc are irrelevant to a
# library package.
cd "$bld"
"$src/configure" \
    --cc="${CC:-cc}" \
    --disable-autodetect \
    --disable-programs \
    --disable-doc

mkdir -p "$gen/libavutil" "$gen/libavcodec" "$gen/libavformat" \
         "$gen/libavfilter" "$gen/libavdevice"

cp config.h config_components.h "$gen/"
if [ -f config.asm ]; then cp config.asm config_components.asm "$gen/"; fi
cp libavutil/avconfig.h "$gen/libavutil/"

# avconfig.h is the ONLY consumer-visible generated header. It is identical on
# every supported little-endian/fast-unaligned target, so one canonical copy
# lives in the platform-neutral, consumer-propagated gen/public/ — verify the
# fresh snapshot agrees rather than silently diverging.
mkdir -p "$root/gen/public/libavutil"
if [ -f "$root/gen/public/libavutil/avconfig.h" ]; then
    cmp "$root/gen/public/libavutil/avconfig.h" libavutil/avconfig.h || {
        echo "gen_config: avconfig.h diverges from gen/public copy for $target" >&2
        exit 1
    }
else
    cp libavutil/avconfig.h "$root/gen/public/libavutil/"
fi
cp libavcodec/codec_list.c libavcodec/parser_list.c libavcodec/bsf_list.c \
   "$gen/libavcodec/"
cp libavformat/demuxer_list.c libavformat/muxer_list.c \
   libavformat/protocol_list.c "$gen/libavformat/"
if [ -f libavfilter/filter_list.c ]; then cp libavfilter/filter_list.c "$gen/libavfilter/"; fi
if [ -f libavdevice/indev_list.c ]; then cp libavdevice/indev_list.c libavdevice/outdev_list.c "$gen/libavdevice/"; fi

# ffversion.h is normally a make-time product; generate it here with a RELATIVE
# output path (an absolute path would leak into the include guard).
sh "$src/ffbuild/version.sh" "$src" libavutil/ffversion.h
cp libavutil/ffversion.h "$gen/libavutil/"

# Derive the exact compiled-source list from make's own dry run — the single
# source of truth for what this configuration builds.
make -n >"$bld/make-n.log" 2>/dev/null || true
python3 "$root/tools/gen_sources.py" "$bld/make-n.log" "$root/mcpp.toml" "$target"

echo "snapshot written to gen/$target/, mcpp.toml sources updated"
