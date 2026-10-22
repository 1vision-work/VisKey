#!/usr/bin/env bash
# Prints the VisKey version used for release file names and the version shown in the apps.
#
#   scripts/viskey_version.sh          -> 1.1.0-beta.5   (tag v1.1.0-beta.5)
#   scripts/viskey_version.sh --deb    -> 1.1.0~beta.5   (Debian form: "-" sorts after the release, "~" before)
#   scripts/viskey_version.sh --numeric -> 1.1.0         (Windows VERSIONINFO, CFBundleVersion)
#
# Source, first match wins:
#   1. $VISKEY_VERSION (with or without a leading "v");
#   2. the v* tag being built in GitHub Actions ($GITHUB_REF_TYPE=tag, $GITHUB_REF_NAME);
#   3. a v* tag pointing exactly at HEAD (local builds);
#   4. the default in linux/ibus/viskey_version.py (non-tag builds).
# A tag or $VISKEY_VERSION that is not x.y.z[-prerelease][+build] is an error.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FORM="${1:-}"
case "$FORM" in
    ""|--deb|--numeric) ;;
    *) echo "usage: $0 [--deb|--numeric]" >&2; exit 2 ;;
esac

RE='^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$'

ver=""
from=""
if [[ -n "${VISKEY_VERSION:-}" ]]; then
    ver="${VISKEY_VERSION#v}"
    from="VISKEY_VERSION"
elif [[ "${GITHUB_REF_TYPE:-}" == "tag" && "${GITHUB_REF_NAME:-}" == v* ]]; then
    ver="${GITHUB_REF_NAME#v}"
    from="tag $GITHUB_REF_NAME"
elif tag="$(git -C "$ROOT_DIR" describe --tags --exact-match --match 'v[0-9]*' HEAD 2>/dev/null)"; then
    ver="${tag#v}"
    from="tag $tag"
else
    ver="$(sed -n 's/^VERSION = "\([^"]*\)".*/\1/p' "$ROOT_DIR/linux/ibus/viskey_version.py")"
    from="linux/ibus/viskey_version.py"
fi

if [[ ! "$ver" =~ $RE ]]; then
    echo "Invalid VisKey version '$ver' (from $from): expected x.y.z[-prerelease][+build]" >&2
    exit 1
fi

case "$FORM" in
    --deb) echo "${ver//-/\~}" ;;
    --numeric) echo "${ver%%[-+]*}" ;;
    *) echo "$ver" ;;
esac
