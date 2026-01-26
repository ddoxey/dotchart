# dotchart (starter)

A small, fast CLI that reads numeric values and prints a Braille-based terminal chart.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
echo "1,10,20,10,1" | ./build/dotchart -F,
```

## Notes

- v0 renders unsigned bars (negatives are clamped to 0 for now).
- Next steps: signed baseline, y-axis labels, formatting, and width-aware titles.
