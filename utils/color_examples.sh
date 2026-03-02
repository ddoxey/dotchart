#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
dotchart_bin="${DOTCHART_BIN:-${script_dir}/../build/dotchart}"
if [[ ! -x "$dotchart_bin" ]]; then
  dotchart_bin="$(command -v dotchart || true)"
fi
if [[ -z "${dotchart_bin}" ]]; then
  echo "dotchart binary not found. Set DOTCHART_BIN or build ./build/dotchart." >&2
  exit 1
fi

echo "Example 1: Symmetric sine + slight trend (bar, axes, warm ramp)"
"${script_dir}/sinewave.sh" | \
  awk '{print $1+0.15*NR/100.0}' | \
  "$dotchart_bin" -y -x --color 196..231 -H 21 -W 80
echo

echo "Example 2: Mixed waves + step shift (bar, axes, neon steps)"
"${script_dir}/sinewave.sh" | \
  awk 'NR%20==0{offset+=0.3} {print $1+offset}' | \
  "$dotchart_bin" -y -x --color 160,167,174,181,188,195,202,209,216,223 -H 21 -W 80
echo

echo "Example 3: Damped oscillation (bar, axes, sunset ramp)"
python - <<'PY' | \
  "$dotchart_bin" -y -x --color 196..208 -H 21 -W 80
import math
for i in range(1, 201):
    t = i / 20.0
    print(math.sin(t) * math.exp(-t / 6.0))
PY
echo

echo "Example 4: Alternating polarity with ramps (bar, axes, crisp bands)"
python - <<'PY' | \
  "$dotchart_bin" -y -x --color 200,205,210,215,220,225,230 -H 21 -W 80
for i in range(1, 181):
    block = (i // 30) % 2
    v = (i % 30) / 30.0
    print(v if block == 0 else -v)
PY
echo

echo "Example 5: Point plot (sine, axes, electric ramp)"
"${script_dir}/sinewave.sh" | \
  "$dotchart_bin" -y -x --style=point --color 39,45,51,87,123,159,195,201 -H 21 -W 80
echo

echo "Example 6: MSFT close history (CSV, axes, warm ramp)"
if [[ -f "${script_dir}/history_MSFT.csv" ]]; then
  awk -F, 'NR>1{print $5}' "${script_dir}/history_MSFT.csv" | \
    "$dotchart_bin" -y --color 196..231 -H 21 -W 80
fi

echo "Example 7: Split palette defaults (positive/negative, auto ranges)"
"${script_dir}/sinewave.sh" | \
  "$dotchart_bin" -y -x --256-color -H 28
echo

echo "Example 8: Split palette override (explicit pos/neg ranges)"
"${script_dir}/sinewave.sh" | \
  "$dotchart_bin" -y -x --256-color-pos 185..170 --256-color-neg 113..98 -H 21
echo
