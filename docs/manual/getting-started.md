# Getting Started

This guide covers prerequisites, building Subvision CV for each platform, and writing your first application.

## Prerequisites

### Common Requirements

- **CMake** 3.27+ ([cmake.org](https://cmake.org/download/))
- **OpenCV** 4.x with core, imgproc, imgcodecs, features2d, calib3d, dnn modules

### For Native C++ Build

- A C++23-compatible compiler (GCC 13+, Clang 16+, or MSVC 2022)
- OpenCV 4.x installed and discoverable by CMake

### For WebAssembly Build

- **Docker** ([docker.com](https://www.docker.com/))
- The build uses the `ghcr.io/subvision-soft/subvision-emscripten:2025.6.1` image which includes Emscripten SDK + OpenCV

### For .NET Build

- **Windows** with Visual Studio 2022
- **OpenCV 4.x** (`choco install opencv`)
- **.NET Framework 4.7.2+** or .NET 6+

---

## Building

### Native C++ Library

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Run tests
cd test
./subvision_tests
```

This produces the static library `subvision_lib` which can be linked into any C++ application.

### WebAssembly (via Docker)

```bash
# Build both standard and ES6 module versions
make all

# Or build individually
make subvision       # Standard version
make subvision_es6   # ES6 module version
```

Output files are placed in `build_wasm/`:
- `subvision_core.js` — Standard WebAssembly wrapper
- `subvision_core_es6.js` — ES6 module wrapper
- `subvision_core.wasm` — WebAssembly binary

### .NET Wrapper (Windows)

```bash
# x64 build
mkdir build-dotnet-x64 && cd build-dotnet-x64
cmake -G "Visual Studio 17 2022" -A x64 -DBUILD_CLI_WRAPPER=ON ..
cmake --build . --config Release

# x86 build
cd ..
mkdir build-dotnet-x86 && cd build-dotnet-x86
cmake -G "Visual Studio 17 2022" -A Win32 -DBUILD_CLI_WRAPPER=ON ..
cmake --build . --config Release
```

Output:
- `build-dotnet-x64/bin/x64/SubvisionNET.dll`
- `build-dotnet-x86/bin/x86/SubvisionNET.dll`

---

## Quick Start

### C++ — Detect Impacts

```cpp
#include "subvision_cv.h"
#include <iostream>

int main() {
    // Load an image of a shooting sheet
    cv::Mat image = cv::imread("target_sheet.jpg");
    if (image.empty()) {
        std::cerr << "Failed to load image" << std::endl;
        return 1;
    }

    // Enable logging for debugging
    subvision::setLoggingEnabled(true);

    // Run the full detection pipeline
    subvision::ImpactResults results;
    bool success = subvision::retrieveImpacts(image, results);

    if (success) {
        std::cout << "Detected " << results.impacts.size() << " impacts:" << std::endl;
        for (const auto& impact : results.impacts) {
            std::cout << "  Zone: " << impact.zone
                      << ", Score: " << impact.score
                      << ", Distance: " << impact.distance << "mm"
                      << ", Angle: " << impact.angle << "°"
                      << std::endl;
        }

        // Save the annotated image
        cv::imwrite("annotated_result.jpg", results.annotatedImage);
    } else {
        std::cerr << "Detection failed" << std::endl;
    }

    return 0;
}
```

### C++ — Two-Step Detection (Manual Sheet Coordinates)

```cpp
#include "subvision_cv.h"

// Step 1: Detect sheet coordinates (can be stored for reuse)
cv::Mat image = cv::imread("target_sheet.jpg");
auto coords = subvision::getSheetCoordinates(image);
// coords are normalised percentages — resolution-independent

// Step 2: Process with known coordinates (faster, no auto-detection)
subvision::ImpactResults results;
subvision::retrieveImpacts(image, results, coords);
```

### C# — .NET Usage

```csharp
using SubvisionNET;

// Load image as RGBA byte array (from your imaging library)
byte[] imageData = LoadImageAsRGBA("target_sheet.jpg", out int width, out int height);

// Optional: enable logging
SubvisionCore.SetLoggingEnabled(true);

// Detect impacts
var results = SubvisionCore.ProcessTargetImage(imageData, width, height, null);

if (results != null)
{
    Console.WriteLine($"Detected {results.Impacts.Count} impacts:");
    foreach (var impact in results.Impacts)
    {
        Console.WriteLine($"  Zone: {impact.Zone}, Score: {impact.Score}, " +
                          $"Distance: {impact.Distance}mm");
    }

    // The annotated image is available as RGBA byte array
    // results.AnnotatedImageData (Width × Height × 4 bytes)
}
```

---

## CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_TESTS` | Build unit tests | `ON` |
| `BUILD_CLI_WRAPPER` | Build C++/CLI .NET wrapper | `OFF` |

## Next Steps

- **[JavaScript / WebAssembly Guide](javascript.md)** — Detailed browser integration
- **[Architecture](architecture.md)** — Understand the system design
