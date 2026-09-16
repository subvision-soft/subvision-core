# Subvision Core

**Subvision Core** is a cross-platform C++ computer vision library powering the [Subvision](https://github.com/subvision-soft) underwater target shooting scoring system.

## Repository structure

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── native/
│   ├── include/
│   ├── src/
│   └── tests/
├── cli/
│   └── src/
├── wasm/
│   ├── emscripten_binding.cpp
│   └── web/
├── dotnet/
│   ├── Subvision.nuspec
│   └── Subvision.targets
└── resources/
```

## Dependency management (vcpkg)

Native C++ dependencies are declared in `vcpkg.json` (manifest mode):

- `opencv`
- `gtest`

Example setup:

```bash
git clone https://github.com/subvision-soft/subvision-core.git
cd subvision-core

git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT="$PWD/vcpkg"
```

On Windows, use `bootstrap-vcpkg.bat` and set `VCPKG_ROOT` accordingly.

## Build workflows

### Native C++ library

```bash
cmake --preset native
cmake --build --preset native
```

### Native C++ tests (CTest)

```bash
cmake --preset native-tests
cmake --build --preset native-tests
ctest --preset native-tests
```

### C++/CLI wrapper (.NET, Windows + MSVC)

```powershell
cmake --preset cli
cmake --build --preset cli --config Release
```

Artifacts are generated under `build/cli/bin/<arch>/` with output name `SubvisionNET.dll`.

### WebAssembly (Emscripten)

#### CMake preset workflow

```bash
# Requires EMSDK environment (EMSDK set)
cmake --preset emscripten
cmake --build --preset emscripten
```

Artifacts are generated in `build/emscripten/build_wasm/` (`subvision.js`, `subvision.mjs`, and `index.html`).

#### Docker/Makefile workflow (existing)

```bash
make all
```

Artifacts are generated in `build_wasm/` (`subvision.js`, `subvision.mjs`, and `index.html`).

## Notes

- The native C++ core is isolated in `native/` and reused by CLI and WASM targets.
- Platform-specific layers (`cli/`, `wasm/`) depend on `subvision_lib`.
- Native tests are isolated from .NET and WASM layers.

## License

MIT License – see [LICENSE](LICENSE) for details.
