#!/bin/zsh

set -euo pipefail

if [[ $# -gt 2 ]]; then
    echo "usage: $0 [build-dir-or-dist-dir] [output-dir]" >&2
    exit 1
fi

script_dir=${0:A:h}
repo_dir=${script_dir:h}
input_dir=${1:-"${repo_dir}/build/dist"}
input_dir=${input_dir:A}
output_dir=${2:-"${repo_dir}/build"}
output_dir=${output_dir:A}

if [[ -d "${input_dir}/Duck.app" ]]; then
    app_path="${input_dir}/Duck.app"
else
    app_path="${input_dir}/dist/Duck.app"
fi

dmg_path="${output_dir}/Duck-macos11-universal.dmg"

if [[ ! -d "${app_path}" ]]; then
    echo "Duck.app was not found at ${app_path}" >&2
    echo "Run: cmake --build ${repo_dir}/build --target dist --parallel" >&2
    exit 1
fi

stage_dir=$(mktemp -d "${TMPDIR:-/tmp}/duck-dmg-stage.XXXXXX")
cleanup() {
    rm -rf "${stage_dir}"
}
trap cleanup EXIT

mkdir -p "${stage_dir}"
rsync -a --delete --exclude=".DS_Store" "${app_path}/" "${stage_dir}/Duck.app/"
ln -s /Applications "${stage_dir}/Applications"

rm -f "${dmg_path}"
hdiutil create \
    -volname "Duck" \
    -srcfolder "${stage_dir}" \
    -format UDZO \
    "${dmg_path}"
hdiutil verify "${dmg_path}"

echo "${dmg_path}"
