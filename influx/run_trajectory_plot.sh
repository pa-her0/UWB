#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_PATH="${1:-$ROOT_DIR/configs/trajectory_plot.json}"
shift $(( $# > 0 ? 1 : 0 ))

source "$ROOT_DIR/.venv/bin/activate"
python -m uwb_pipeline.trajectory_plot --config "$CONFIG_PATH" "$@"
