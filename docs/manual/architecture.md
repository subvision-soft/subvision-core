# Architecture

This document describes the architecture of Subvision CV, including the module structure, processing pipeline, and cross-platform design.

## System Overview

Subvision CV is a C++ computer vision library built on OpenCV. It processes photographs of underwater target shooting sheets to detect impacts and compute scores.

```
┌─────────────────────────────────────────────────────┐
│                 Application Layer                    │
│   ┌──────────┐  ┌──────────────┐  ┌──────────────┐  │
│   │ C++ App  │  │  Browser     │  │  .NET App    │  │
│   │ (native) │  │  (WASM/JS)   │  │  (C#/VB)     │  │
│   └────┬─────┘  └──────┬───────┘  └──────┬───────┘  │
├────────┼────────────────┼────────────────┼───────────┤
│        │   Platform Binding Layer        │           │
│   ┌────┴─────┐  ┌──────┴───────┐  ┌─────┴──────┐   │
│   │ Direct   │  │  Emscripten  │  │  C++/CLI   │   │
│   │ C++ Link │  │  embind      │  │  Wrapper   │   │
│   └────┬─────┘  └──────┬───────┘  └─────┬──────┘   │
├────────┼────────────────┼────────────────┼───────────┤
│        └────────────────┼────────────────┘           │
│                         │                            │
│              ┌──────────┴──────────┐                 │
│              │   Subvision CV Core │                 │
│              │     (C++ / OpenCV)  │                 │
│              └─────────────────────┘                 │
└─────────────────────────────────────────────────────┘
```

## Module Structure

### Core Modules

| Module | Header | Responsibility |
|--------|--------|---------------|
| **Types** | `types.h` | Core data structures: `Impact`, `ImpactResults`, `Ellipse` |
| **Constants** | `constants.h` | Zone IDs, processing dimensions, kernels |
| **Utils** | `utils.h` | Math, geometry, coordinate transforms, scoring |
| **Image Processing** | `image_processing.h` | Contour analysis, colour masking, ellipse fitting |
| **Sheet Detection** | `sheet_detection.h` | Sheet boundary detection, perspective correction |
| **Target Detection** | `target_detection.h` | Target ring localisation, multi-zone detection |
| **Impact Detection** | `impact_detection.h` | Impact localisation, scoring, annotation |
| **Logging** | `logging.h` | Cross-platform logging (console, emscripten, .NET) |

### Platform Bindings

| File | Platform | Technology |
|------|----------|-----------|
| `emscripten_binding.cpp` | WebAssembly | Emscripten embind |
| `cli_wrapper.cpp` | .NET | C++/CLI (MSVC) |

### Dependency Graph

```
impact_detection
├── sheet_detection
│   ├── image_processing
│   │   ├── constants
│   │   ├── utils
│   │   └── logging
│   └── utils
├── target_detection
│   ├── image_processing
│   ├── constants
│   ├── utils
│   └── logging
├── image_processing
├── utils
└── logging
```

## Processing Pipeline

### Main Pipeline (`retrieveImpacts`)

```
Input Image (BGR)
       │
       ▼
┌──────────────────┐
│  Sheet Detection  │  getSheetPicture() or getSheetPictureManually()
│  - HLS lightness  │
│  - Contour find   │
│  - Quadrilateral  │
│    validation      │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│   Perspective     │  getPerspectiveTransform + warpPerspective
│   Correction      │  → 2000×2000 flat image
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  Target Detection │  getTargetsEllipse() × 5 zones
│  - XYZ colour     │  (top-left, top-right, bottom-left,
│  - Threshold      │   bottom-right, center)
│  - Ellipse fit    │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  Impact Detection │  getImpactsCoordinates()
│  - CLAHE normalise│
│  - HSV red mask   │
│  - Morphology     │
│  - Ellipse fit    │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  Score Computation│  drawAndGetImpactsPoints()
│  - Zone matching  │
│  - Distance calc  │
│  - Score lookup   │
│  - Annotation     │
└────────┬─────────┘
         │
         ▼
Output: ImpactResults
  - annotatedImage (BGR)
  - impacts[] (distance, score, zone, angle)
```

### Target Zone Layout

A standard underwater shooting sheet has five targets arranged as follows:

```
┌───────────────────────────────────────┐
│                                       │
│  ┌─────────┐         ┌─────────┐     │
│  │  Zone 0  │         │  Zone 1  │    │
│  │ TOP LEFT │         │TOP RIGHT │    │
│  └─────────┘         └─────────┘     │
│                                       │
│           ┌─────────┐                 │
│           │  Zone 4  │                │
│           │  CENTER  │                │
│           └─────────┘                 │
│                                       │
│  ┌─────────┐         ┌─────────┐     │
│  │  Zone 2  │         │  Zone 3  │    │
│  │ BOT LEFT │         │BOT RIGHT │    │
│  └─────────┘         └─────────┘     │
│                                       │
└───────────────────────────────────────┘
```

### Scoring System

The scoring follows FFESSM underwater target shooting rules:

| Distance from Center | Score Formula | Example |
|---------------------|---------------|---------|
| > 48 mm | 0 | — |
| ≤ 0 mm (bullseye) | 570 | 570 |
| 1–5 mm | 570 − (distance × 6) | 3mm → 552 |
| > 5 mm | 540 − ((distance − 5) × 3) | 20mm → 495 |

The real-world distance is calculated by mapping the pixel distance between the target center and the impact to the known 45mm target radius.

## Cross-Platform Design

### Colour Space Handling

Each platform provides images in different colour formats:

| Platform | Input Format | Internal Format |
|----------|-------------|----------------|
| Native C++ | BGR (OpenCV default) | BGR |
| WebAssembly | RGBA (browser Canvas) | BGR (converted) |
| .NET | RGBA (managed bitmap) | BGR (converted) |

All platform bindings convert to BGR before calling the core pipeline. Output images are converted back to RGBA for browser and .NET consumers.

### Memory Management

| Platform | Strategy |
|----------|----------|
| Native C++ | Automatic (stack/RAII) — cv::Mat uses reference counting |
| WebAssembly | Emscripten manages memory; `embind` handles marshalling |
| .NET | `pin_ptr` for input; `Marshal::Copy` for output; GC handles managed objects |

### Logging

The logging module (`logging.h` / `logging.cpp`) uses compile-time preprocessor directives to select the output mechanism:

| Build Target | Preprocessor | Output |
|-------------|-------------|--------|
| Emscripten | `__EMSCRIPTEN__` | `emscripten_log()` → browser console |
| C++/CLI | `_MANAGED` | `System::Console::WriteLine()` |
| Native C++ | (default) | `std::cout` |

## Build System

The project uses CMake with platform-specific configurations:

- **Native**: Standard CMake build with `subvision_lib` static library
- **Emscripten**: Docker-based build with Makefile wrapper
- **C++/CLI**: MSVC-only build with `/clr` and `/EHa` flags

The `CMakeLists.txt` uses conditional logic (`if(EMSCRIPTEN)`, `if(BUILD_CLI_WRAPPER)`) to configure each target independently.
