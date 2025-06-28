# Subvision Core

**Subvision Core** is the WebAssembly (WASM) build of the core C++ computer visions algorithms powering
the [Subvision](https://github.com/subvision-soft) underwater target shooting scoring system. This core is designed to
run efficiently in web environments using [Emscripten](https://emscripten.org/).

Subvision is a mobile and web app developed to detect, locate, and score impacts on underwater target shooting sheets.
It aims to be validated by the **FFESSM** (French Underwater Federation) for official competition use.

---

## 🚀 Features

- C++ core logic for:
    - Sheet detection
    - Target detection
    - Impact localization
    - Score computation
- WebAssembly build for browser usage
- Dockerized build environment for consistency

---

## 📁 Repository Structure
```
├── include/ # Header files
├── src/ # C++ core source files
├── web/ # HTML test interface
├── emscripten_binding.cpp # Emscripten-specific glue code
├── Makefile # Build system
└── build_wasm/ # Output folder (generated)
```

---

## ⚙️ Prerequisites

- [Docker](https://www.docker.com/)
- Internet access to pull the image `ghcr.io/subvision-soft/subvision-emscripten:2025.6.1`

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

Run the following commands depending on your target:

```bash
# Build both versions (standard + ES6)
make all

# Build the standard WebAssembly version
make subvision

# Build the ES6 module WebAssembly version
make subvision_es6
```

The output will be located in the build_wasm/ directory.


---

## 📦 Output Files

| File                  | Description                            |
|-----------------------|----------------------------------------|
| subvision_core.js     | WebAssembly wrapper (standard version) |
| subvision_core_es6.js | WebAssembly wrapper using ES6 modules  |
| index.html            | Test interface for running in browser  |

---

## 🛠️ Makefile Targets

| Target        | Description                          |
|---------------|--------------------------------------|
| all           | Build both standard and ES6 versions |
| subvision     | Build standard WebAssembly version   |
| subvision_es6 | Build ES6 module WebAssembly version |
| help          | Show help message                    |

## 📄 License

MIT License – see [LICENSE](LICENSE) for details.
