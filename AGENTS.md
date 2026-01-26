# AGENTS.md

## Overview

This repository contains **dotchart**, a high-performance command-line utility for rendering numeric data as compact, Unicode-based charts (primarily using Braille characters).

The project is designed to:
- be fast enough for use in pipelines
- behave predictably in non-interactive environments
- follow UNIX CLI conventions
- remain easy to reason about and debug

Contributions and automated assistance are welcome, provided they respect the design principles below.

---

## Design Principles

Agents (human or automated) working on this project should adhere to the following principles:

### 1. Correctness over cleverness
Rendering logic should favor correctness, debuggability, and mathematical clarity over compactness or novelty.

If behavior is ambiguous, add instrumentation (debug output) rather than hiding complexity.

### 2. Minimal visual artifacts
Visual elements such as axes, baselines, and grid boundaries should be:
- minimal in size
- visually unobtrusive
- semantically meaningful

For example:
- the zero baseline in signed mode is a *boundary*, not a full row
- resampling preserves extrema rather than averages when possible

### 3. Explicit semantics
Rendering decisions should be driven by explicit semantics:
- signed vs unsigned data
- baseline placement
- scaling behavior

Avoid “magic” heuristics that change behavior without visibility.

---

## Coding Standards

### Language & Tooling
- **C++20** is the baseline
- Prefer standard library facilities
- Avoid unnecessary dependencies

### Style
- Favor clarity over brevity
- Use small helper functions rather than deeply nested logic
- Prefer `const` correctness
- Avoid premature optimization unless justified by profiling

### Error Handling
- Fail loudly for invalid CLI usage
- Fail quietly (but predictably) for malformed input streams
- Debug information should go to **stderr**, never stdout

---

## CLI & UX Expectations

Agents modifying CLI behavior should follow these conventions:

- Be friendly to Bash / UNIX users
- Prefer short, memorable flags (`-W`, `-F`, `-d`)
- Long flags should be descriptive (`--width`, `--debug`)
- Output to stdout must remain pipe-safe
- Diagnostic or debug output must go to stderr

---

## Rendering & Math Rules

### Resampling
- Unsigned mode: max-bin resampling is acceptable
- Signed mode: preserve sign and magnitude (use extreme-abs semantics)

### Braille Grid
- Each character column represents **two samples**
- Vertical resolution is **4 pixels per cell**
- The zero baseline is a **pixel boundary**, not a row

### Signed Mode
- Positive values render upward from the baseline
- Negative values render downward from the baseline
- Baseline location is determined by data extrema unless forced

---

## Debugging & Instrumentation

The project intentionally supports internal introspection via `--debug`.

Agents should:
- extend debug output when adding non-trivial behavior
- include dimensions, scaling factors, and derived quantities
- avoid removing debug fields unless they are truly obsolete

---

## Tests & Validation

This project does not currently include a formal test suite.

Instead, correctness is validated through:
- deterministic signal generators (e.g., sine waves)
- visual symmetry and extrema checks
- debug table inspection

Agents should preserve this workflow and avoid introducing nondeterminism.

---

## What *Not* to Do

Agents should avoid:
- adding heavyweight frameworks
- introducing hidden state
- auto-detecting behavior without user visibility
- reformatting output in ways that break existing pipelines

---

## Final Note

dotchart is intentionally small, explicit, and inspectable.

If a change makes the code harder to reason about, it’s probably the wrong change.

When in doubt, favor transparency.
