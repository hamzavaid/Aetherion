# Aetherion

Aetherion is an interactive, deterministic 3D mechanics simulator built around a testable
double-precision physics core. The current release supports Newtonian gravity, multiple numerical
integrators, engineering diagnostics, and an optional OpenGL desktop interface.

## Build

Requirements: CMake 3.25+, C and C++20 compilers, Git, Python 3 with Jinja2 (for the pinned
OpenGL loader generator), and an internet connection for pinned dependencies.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

On Windows with MSYS2/UCRT64 GCC, keep it separate from MSVC and WSL build directories:

```powershell
cmake -S . -B build/windows-gcc -G Ninja `
  -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
cmake --build build/windows-gcc --parallel
ctest --test-dir build/windows-gcc --output-on-failure
```

Use `-DAETHERION_BUILD_RENDERER=OFF` when configuring a headless build.

The desktop camera starts above the engineering grid looking down. It uses left-drag to orbit,
middle/right-drag to pan, the mouse wheel to zoom, and `R` to reset. The camera-centered grid expands
with zoom and changes spacing in 1/2/5 engineering increments so it continues across the visible
scene. Simulation coordinates remain SI-valued doubles; rendering scale and apparent body radius are
visualization-only values.

The dockable engineering workspace provides a scene hierarchy, validated property inspector,
transport controls, global gravity/time settings, and live diagnostics. Scene and solver edits are
queued and applied at deterministic simulation boundaries; rejected numeric input appears in the
diagnostic command log.

Selecting a body in the hierarchy highlights it in gold in the 3D scene. Object trails can be
enabled under Simulation Controls, with a logarithmic duration slider measured in simulation
seconds. Trail history is bounded and automatically clears when simulation time is reset.

Semi-Implicit Euler, Velocity Verlet, and classical RK4 are selectable at runtime. The diagnostics
and plots report mechanical energy and momentum errors relative to the initial sample, while the
timestep analyzer warns when orbital or pair-crossing timescales are under-resolved. Run
`aetherion_compare` for the reproducible one-period Earth-like integrator comparison.
