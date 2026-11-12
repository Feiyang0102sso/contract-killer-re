#!/bin/bash
# One-click build inside the build Linux (run as root after setup_distro.sh):
# compile every source (incremental) and link BountyHunter into <repo>/_output.
# Usage: build.sh [-B] [-c]
#   -B  recompile every object even if it is up to date
#   -c  compile only, skip the link (the link fails while sources are still missing)
set -euo pipefail

TOOLCHAIN=$(cd "$(dirname "$0")" && pwd)

FORCE=""
LINK=1
for argument in "$@"; do
    case "$argument" in
        -B) FORCE="-B" ;;
        -c) LINK=0 ;;
        *) echo "usage: build.sh [-B] [-c]" >&2; exit 2 ;;
    esac
done

if [ "$(id -u)" != 0 ]; then
    echo "build.sh: run as root (the build bind-mounts the original source path)" >&2
    exit 1
fi
AS=/work/xcode421/packages/DeveloperToolsCLI/usr/libexec/as/arm/as
if [ ! -x /work/maloader_src/maloader/ld-mac ] || [ ! -x /work/xcode421/packages/clang/usr/bin/clang ] || [ ! -x "$AS" ]; then
    echo "build.sh: toolchain missing, run setup first (setup.bat or toolchain/setup_distro.sh)" >&2
    exit 1
fi

echo "== compile"
bash "$TOOLCHAIN/build_ck.sh" -q $FORCE --all
if [ "$LINK" = 1 ]; then
    echo "== link"
    bash "$TOOLCHAIN/link_ck.sh"
fi
