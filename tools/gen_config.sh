#!/bin/sh
# gen_config.sh — maintainer-time descriptor pipeline for compat.ffmpeg.
#
# Fetches the pinned OFFICIAL FFmpeg tarball (tools/fetch_upstream.sh), runs
# ./configure ONCE (out-of-tree, hermetic full profile), captures every
# generated file plus the `make -n` source list, and emits the mcpp-index
# descriptor via tools/gen_descriptor.py. Consumers never run this: FFmpeg
# reaches them as the compat.ffmpeg package.
#
# Usage: tools/gen_config.sh [target] [out.lua]
#   target  default: autodetected <os>-<arch>   (only linux-x86_64 supported yet)
#   out.lua default: ../mcpp-index/pkgs/c/compat.ffmpeg.lua (if that repo is
#           checked out next to this one), else ./compat.ffmpeg.lua
set -eu

target="${1:-$(uname -s | tr 'A-Z' 'a-z' | sed s/darwin/macos/)-$(uname -m | sed s/arm64/aarch64/)}"
root="$(cd "$(dirname "$0")/.." && pwd)"
version="${FFMPEG_VERSION:-8.1.2}"
sha256="${FFMPEG_SHA256:-32faba5ef67340d54724941eae1425580791195312a4fd13bf6f820a2818bf22}"
default_out="$root/../mcpp-index/pkgs/c/compat.ffmpeg.lua"
[ -d "$(dirname "$default_out")" ] || default_out="$root/compat.ffmpeg.lua"
out="${2:-$default_out}"

src="$(sh "$root/tools/fetch_upstream.sh")"
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

# ffversion.h is normally a make-time product; generate it here with a RELATIVE
# output path (an absolute path would leak into the include guard).
sh "$src/ffbuild/version.sh" "$src" libavutil/ffversion.h

# The `make -n` dry run is the ground truth for which .c/.S/.asm files this
# frozen configuration compiles (CONFIG_* gating + _select closures resolved).
make -n > make-n.log 2>/dev/null || true
grep -oE '(\.\./[^ ]+|src)/[A-Za-z0-9_/.+-]+\.(c|S|asm)\b' make-n.log \
    | sed "s|^\.\./ffmpeg-$version/|src/|; s|^$src/|src/|" \
    | grep '^src/' | sort -u > sources.txt

python3 "$root/tools/gen_descriptor.py" "$bld" "$version" "$sha256" "$out"
echo "descriptor written: $out (configured on: $target)"
