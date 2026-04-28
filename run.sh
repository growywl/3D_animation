#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LOGL_ROOT_PATH="$ROOT_DIR"

exec "$ROOT_DIR/bin/8.guest/2020/skeletal_animation/8.guest_2020_skeletal_animation"

