# Subvision CV Documentation

Welcome to the Subvision CV manual. This documentation covers the complete API and usage guides for the cross-platform computer vision library.

## Overview

**Subvision CV** is a C++ computer vision library for detecting, locating, and scoring impacts on underwater target shooting sheets. It powers the [Subvision](https://github.com/subvision-soft) application used for FFESSM competition scoring.

### Core Capabilities

| Feature | Description |
|---------|-------------|
| **Sheet Detection** | Automatically locate the shooting sheet in a photograph |
| **Target Detection** | Identify the five concentric ring targets on the sheet |
| **Impact Localisation** | Detect red impact marks (shots) on the targets |
| **Score Computation** | Calculate scores using FFESSM federation rules (0–570 scale) |
| **Image Annotation** | Generate annotated images with targets, impacts, and scores drawn |

### Platform Support

The library is deployable on three platforms from a single C++ codebase:

- **Native C++** — Static library linked via CMake
- **WebAssembly** — ES6 module built with Emscripten for browser usage
- **.NET** — C++/CLI managed wrapper for Windows desktop apps

### Processing Pipeline

The library follows a sequential pipeline:

1. **Input** — Receive a photograph of a shooting sheet (RGBA or BGR)
2. **Sheet Detection** — Detect the white sheet boundary and extract corner coordinates
3. **Perspective Correction** — Warp to a flat 2000×2000 pixel image
4. **Target Detection** — Locate the elliptical target rings in each of the 5 zones
5. **Impact Detection** — Find red impact marks via colour analysis
6. **Scoring** — Compute distance and score for each impact
7. **Annotation** — Draw targets, impacts, lines, and scores on the image
8. **Output** — Return the annotated image and list of scored impacts

## Next Steps

- **[Getting Started](getting-started.md)** — Installation and first use
- **[JavaScript / WebAssembly](javascript.md)** — Browser integration guide
- **[Architecture](architecture.md)** — Detailed system design
