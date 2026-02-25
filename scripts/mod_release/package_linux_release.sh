#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  package_linux_release.sh --release-dir <dir> [--zip-path <zip>]

Description:
  Prepares a Linux mod release directory for end-user extraction by replacing
  symlinked shared libraries with real files, validating shared objects, and
  creating a zip archive.
EOF
}

release_dir=""
zip_path=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release-dir)
      release_dir="${2:-}"
      shift 2
      ;;
    --zip-path)
      zip_path="${2:-}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -z "$release_dir" ]]; then
  usage >&2
  exit 1
fi

release_dir="$(cd "$release_dir" && pwd)"
if [[ ! -d "$release_dir" ]]; then
  echo "Release directory not found: $release_dir" >&2
  exit 1
fi

if [[ -z "$zip_path" ]]; then
  zip_path="${release_dir}.zip"
fi
zip_path="$(mkdir -p "$(dirname "$zip_path")" && cd "$(dirname "$zip_path")" && pwd)/$(basename "$zip_path")"

for required in barony editor run-barony.sh README.txt libsteam_api.so; do
  if [[ ! -e "$release_dir/$required" ]]; then
    echo "Missing required release file: $release_dir/$required" >&2
    exit 1
  fi
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"$script_dir/flatten_linux_so_symlinks.sh" "$release_dir"

if find "$release_dir" -maxdepth 1 -type l -name 'lib*.so*' | grep -q .; then
  echo "Refusing to package: symlinked shared libraries still present" >&2
  find "$release_dir" -maxdepth 1 -type l -name 'lib*.so*' >&2
  exit 1
fi

bad=0
while IFS= read -r sofile; do
  [[ -f "$sofile" ]] || continue
  if ! head -c 4 "$sofile" | grep -q $'^\x7fELF'; then
    echo "Non-ELF shared library payload: $sofile" >&2
    bad=1
  fi
done < <(find "$release_dir" -maxdepth 1 -type f -name 'lib*.so*' | sort)

if [[ "$bad" -ne 0 ]]; then
  exit 1
fi

if find "$release_dir" -maxdepth 1 -type f -name 'lib*.so*' -size -4096c | grep -q .; then
  echo "Refusing to package: tiny shared library files detected (<4KB)" >&2
  find "$release_dir" -maxdepth 1 -type f -name 'lib*.so*' -size -4096c >&2
  exit 1
fi

rm -f "$zip_path"
(
  cd "$(dirname "$release_dir")"
  zip -qry "$zip_path" "$(basename "$release_dir")"
)

echo "Packaged Linux release:"
echo "  release_dir=$release_dir"
echo "  zip_path=$zip_path"
