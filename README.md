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

The desktop camera starts above the engineering grid looking down. Left-click selects a rendered
body, left-drag orbits, middle/right-drag pans, the mouse wheel zooms, and `R` resets the camera.
Camera controls are suspended whenever a UI window or widget owns the corresponding input. The
camera-centered grid expands with zoom, changes spacing in 1/2/5 engineering increments, and can be
hidden from Simulation Controls.

Simulation coordinates remain SI-valued doubles. Meters per render unit, minimum apparent radius,
and the logarithmic body-radius multiplier are visualization-only controls and never modify physical
radius, collision geometry, gravity, or integration. Direct scene picking follows the displayed
radius, making visually enlarged astronomical bodies selectable.

The dockable engineering workspace provides a scene hierarchy, validated property inspector,
transport controls, global gravity/time settings, and live diagnostics. Scene and solver edits are
queued and applied at deterministic simulation boundaries; rejected numeric input appears in the
diagnostic command log.

**Save** creates an in-session checkpoint of the complete physical scene, simulation time, and
runtime solver settings. **Reset** restores the latest checkpoint, or the startup preset when no
checkpoint exists. Save History lists up to 64 checkpoints in newest-first order and can restore any
retained state; restoring pauses the simulation and establishes a fresh telemetry baseline.

Selecting a body in the hierarchy or directly in the scene highlights it in gold and displays it in
the inspector. Object trails can be enabled under Simulation Controls, with a logarithmic duration
slider measured in simulation seconds. Trail history is bounded and automatically clears when
simulation time is reset.

Semi-Implicit Euler, Velocity Verlet, and classical RK4 are selectable at runtime. The diagnostics
and plots report mechanical energy and momentum errors relative to the initial sample, while the
timestep analyzer warns when orbital or pair-crossing timescales are under-resolved. Run
`aetherion_compare` for the reproducible one-period Earth-like integrator comparison.
