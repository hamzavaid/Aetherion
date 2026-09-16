# Aetherion

Aetherion is an interactive, deterministic 3D mechanics simulator built around a testable
double-precision physics core. The current release supports Newtonian gravity, multiple numerical
integrators, engineering diagnostics, and an optional OpenGL desktop interface.

## Build

Requirements: CMake 3.25+, a C++20 compiler, Git, and an internet connection for pinned dependencies.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Use `-DAETHERION_BUILD_RENDERER=OFF` when configuring a headless build.
