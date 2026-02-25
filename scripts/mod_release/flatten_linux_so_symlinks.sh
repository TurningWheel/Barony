#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 /path/to/linux-release-dir" >&2
  exit 1
fi

release_dir="$1"
if [[ ! -d "$release_dir" ]]; then
  echo "Release directory not found: $release_dir" >&2
  exit 1
fi

shopt -s nullglob
count=0

for link in "$release_dir"/lib*.so*; do
  [[ -L "$link" ]] || continue

  target_rel="$(readlink "$link")"
  if [[ -z "$target_rel" ]]; then
    echo "Skipping unresolved symlink: $link" >&2
    continue
  fi

  if [[ "$target_rel" = /* ]]; then
    target_path="$target_rel"
  else
    target_path="$release_dir/$target_rel"
  fi

  if [[ ! -e "$target_path" ]]; then
    echo "Skipping unresolved symlink target: $link -> $target_rel" >&2
    continue
  fi

  rm -f "$link"
  cp -f "$target_path" "$link"
  chmod 0644 "$link" || true
  ((count += 1))
done

echo "Flattened $count symlinked shared libraries in $release_dir"
