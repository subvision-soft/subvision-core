# Subvision Core

**Subvision Core** is a cross-platform C++ computer vision library powering
the [Subvision](https://github.com/subvision-soft) underwater target shooting scoring system.

Subvision is a mobile and web app developed to detect, locate, and score impacts on underwater target shooting sheets.
It aims to be validated by the **FFESSM** (French Underwater Federation) for official competition use.

---

## 🚀 Features

- C++ core logic for:
    - Sheet detection
    - Target detection
    - Impact localization
    - Score computation
- **Multiple platform support:**
    - WebAssembly build for browser usage
    - .NET bindings via C++/CLI for Windows applications
    - Native C++ library
- Dockerized build environment for consistency

---

## 📁 Repository Structure
```
├── include/                # Header files
├── src/                    # C++ core source files
├── test/                   # Unit tests
├── web/                    # HTML test interface
├── emscripten_binding.cpp  # Emscripten JavaScript bindings
├── cli_wrapper.cpp         # C++/CLI .NET bindings
├── CMakeLists.txt          # CMake build configuration
├── Makefile                # WebAssembly build system
└── build_wasm/             # WebAssembly output (generated)
```

---

## ⚙️ Prerequisites

### For WebAssembly Build:
- [Docker](https://www.docker.com/)
- Internet access to pull the image `ghcr.io/subvision-soft/subvision-emscripten:2025.6.1`

### For .NET Build:
- Windows with Visual Studio 2022
- CMake 3.30.5+
- OpenCV 4.x (installed via chocolatey: `choco install opencv`)
- .NET Framework 4.7.2+ or .NET 6+

---

## 🐳 Docker Image

Builds use the `ghcr.io/subvision-soft/subvision-emscripten:2025.6.1` Docker image, which is maintained in
the [Subvision Stack project](https://github.com/subvision-soft/subvision-stack).

This image includes:

- Emscripten SDK
- OpenCV with pkg-config
- CMake and other tools required for building C++ and WASM

---

## 🔨 How to Build

### WebAssembly Build

Run the following commands depending on your target:

```bash
# Build both versions (standard + ES6)
make all

# Build the standard WebAssembly version
make subvision

# Build the ES6 module WebAssembly version
make subvision_es6
```

The output will be located in the `build_wasm/` directory.

### .NET Build (Windows)

```bash
# Build the C++/CLI .NET wrapper
mkdir build-dotnet
cd build-dotnet
cmake -G "Visual Studio 17 2022" -A x64 -DBUILD_CLI_WRAPPER=ON ..
cmake --build . --config Release
```

The output `SubvisionNET.dll` will be located in `build-dotnet\bin\Release\`.

### Native C++ Build

```bash
# Build native library and tests
mkdir build
cd build
cmake ..
cmake --build . --config Release

# Run tests
cd test
./subvision_tests
```

---

## 📦 Output Files

### WebAssembly

| File                  | Description                            |
|-----------------------|----------------------------------------|
| subvision_core.js     | WebAssembly wrapper (standard version) |
| subvision_core_es6.js | WebAssembly wrapper using ES6 modules  |
| index.html            | Test interface for running in browser  |

### .NET

| File              | Description                         |
|-------------------|-------------------------------------|
| SubvisionNET.dll  | .NET assembly with C++/CLI bindings |

---

## 💻 Usage Examples

### JavaScript (WebAssembly)

```javascript
import SubvisionCV from './subvision_core_es6.js';

const module = await SubvisionCV();

// Process target image
const results = module.processTargetImage(width, height, imageData);
console.log('Impacts:', results.impacts);

// Get sheet coordinates
const coords = module.getSheetCoordinates(width, height, imageData);
console.log('Corners:', coords);
```

### C# (.NET)

```c++
using SubvisionNET;

// Load image as RGBA byte array
byte[] imageData = LoadImageAsRGBA("target.jpg", out int width, out int height);

// Process target to detect impacts
var results = SubvisionCore.ProcessTargetImage(imageData, width, height);
foreach (var impact in results.Impacts)
{
    Console.WriteLine($"Impact: Distance={impact.Distance}, Score={impact.Score}");
}

// Get sheet coordinates
var coords = SubvisionCore.GetSheetCoordinates(imageData, width, height);
```

---

## 🛠️ Build Targets

### Makefile (WebAssembly)

| Target        | Description                          |
|---------------|--------------------------------------|
| all           | Build both standard and ES6 versions |
| subvision     | Build standard WebAssembly version   |
| subvision_es6 | Build ES6 module WebAssembly version |
| help          | Show help message                    |

### CMake Options

| Option              | Description                     | Default |
|---------------------|---------------------------------|---------|
| BUILD_TESTS         | Build unit tests                | ON      |
| BUILD_CLI_WRAPPER   | Build C++/CLI .NET wrapper      | OFF     |
| EMSCRIPTEN          | Build for WebAssembly           | OFF     |

## 📄 License

MIT License – see [LICENSE](LICENSE) for details.
