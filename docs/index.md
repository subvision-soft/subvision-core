---
uid: index
title: Subvision CV Documentation
---

# Subvision CV Documentation

Welcome to the **Subvision CV** documentation — the complete reference for the cross-platform computer vision library powering the [Subvision](https://github.com/subvision-soft) underwater target shooting scoring system.

## What is Subvision CV?

Subvision CV is a C++ library that detects, locates, and scores impacts on underwater target shooting sheets. It is used by the Subvision mobile and web application, which aims to be validated by the **FFESSM** (French Underwater Federation) for official competition use.

## Platform Support

Subvision CV runs on three platforms from a single C++ codebase:

| Platform | Technology | Output |
|----------|-----------|--------|
| **Native C++** | OpenCV + CMake | Static library (`subvision_lib`) |
| **WebAssembly** | Emscripten + embind | ES6 module (`subvision_core_es6.js`) |
| **.NET** | C++/CLI wrapper | Managed DLL (`SubvisionNET.dll`) |

## Documentation Sections

- **[Getting Started](manual/getting-started.md)** — Prerequisites, build instructions, and quick start guide
- **[JavaScript / WebAssembly Guide](manual/javascript.md)** — Complete guide to using Subvision CV in the browser
- **[Architecture](manual/architecture.md)** — System design, module overview, and data flow
- **[C++ API Reference](cpp/html/index.html)** — Full Doxygen-generated C++ API documentation
- **[.NET API Reference](api/)** — DocFX-generated .NET API documentation

## Quick Examples

### C++

```cpp
#include "subvision_cv.h"

cv::Mat image = cv::imread("target_sheet.jpg");
subvision::ImpactResults results;

if (subvision::retrieveImpacts(image, results)) {
    for (const auto& impact : results.impacts) {
        std::cout << "Score: " << impact.score
                  << ", Distance: " << impact.distance << "mm"
                  << std::endl;
    }
}
```

### JavaScript (WebAssembly)

```javascript
import SubvisionCV from './subvision_core_es6.js';

const module = await SubvisionCV();

const canvas = document.getElementById('targetCanvas');
const ctx = canvas.getContext('2d');
const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

const results = module.processTargetImage(
    canvas.width, canvas.height, imageData.data
);

for (let i = 0; i < results.impacts.size(); i++) {
    const impact = results.impacts.get(i);
    console.log(`Score: ${impact.score}, Distance: ${impact.distance}mm`);
}
```

### C# (.NET)

```csharp
using SubvisionNET;

byte[] imageData = LoadImageAsRGBA("target_sheet.jpg", out int w, out int h);
var results = SubvisionCore.ProcessTargetImage(imageData, w, h, null);

foreach (var impact in results.Impacts)
{
    Console.WriteLine($"Score: {impact.Score}, Distance: {impact.Distance}mm");
}
```
