#!/usr/bin/env bash
set -euo pipefail

# utils/sinewave.sh
# Generate sine wave numeric data for testing and demos.
#
# Defaults (no args):
#   amplitude=1, offset=0, phase=0, cycles=1, points=100
# Meaning:
#   cycles = number of sine cycles across the generated points.

# Defaults
range=""                # optional MIN,MAX (used only to auto-derive offset/amplitude if given)
offset="0"
amplitude="1"
points=100
cycles=1
phase=0
format="%.6f\n"

usage() {
cat <<'EOF'
Usage: sinewave.sh [OPTIONS]

Generate sine wave numeric data for testing and demos.

Equation:
  y[i] = offset + amplitude * sin(2*pi*cycles*i/points + phase)

Options:
  --range MIN,MAX        Optional. If provided and --offset/--amplitude not explicitly set,
                         offset defaults to midpoint and amplitude defaults to half-range.
  --offset VALUE         Vertical offset (default: 0)
  --amplitude VALUE      Amplitude (default: 1)
  --points N             Number of points (default: 100)
  --cycles N             Number of sine cycles across the generated points (default: 1)
  --phase VALUE          Phase offset in radians (default: 0)
  --format FMT           awk printf-style format (default: "%.6f\n")
  -h, --help             Show this help

Examples:
  sinewave.sh
  sinewave.sh --amplitude 45 --offset 50 --cycles 1 --points 100
  sinewave.sh --range 5,95 --cycles 2 | dotchart -H 8 -y
EOF
}

# Track whether user explicitly set these (so --range can fill in only if unset)
offset_set=0
amplitude_set=0

# Parse args
while [[ $# -gt 0 ]]; do
  case "$1" in
    --range)
      range="${2:-}"; shift 2;;
    --offset)
      offset="${2:-}"; offset_set=1; shift 2;;
    --amplitude)
      amplitude="${2:-}"; amplitude_set=1; shift 2;;
    --points)
      points="${2:-}"; shift 2;;
    --cycles)
      cycles="${2:-}"; shift 2;;
    --phase)
      phase="${2:-}"; shift 2;;
    --format)
      format="${2:-}"; shift 2;;
    -h|--help)
      usage; exit 0;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1;;
  esac
done

# If --range provided, optionally derive offset/amplitude (only if not explicitly set)
if [[ -n "$range" ]]; then
  IFS=',' read -r rmin rmax <<< "$range"
  if [[ $offset_set -eq 0 ]]; then
    offset="$(awk -v a="$rmin" -v b="$rmax" 'BEGIN{print (a+b)/2}')"
  fi
  if [[ $amplitude_set -eq 0 ]]; then
    amplitude="$(awk -v a="$rmin" -v b="$rmax" 'BEGIN{print (b-a)/2}')"
  fi
fi

# Generate
awk -v pts="$points" \
    -v off="$offset" \
    -v amp="$amplitude" \
    -v cyc="$cycles" \
    -v ph="$phase" \
    -v fmt="$format" '
BEGIN {
  pi = 3.141592653589793
  for (i = 0; i < pts; i++) {
    x = (2 * pi * cyc * i / pts) + ph
    y = off + amp * sin(x)
    printf fmt, y
  }
}'
