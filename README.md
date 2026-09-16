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

Use `-DAETHERION_BUILD_RENDERER=OFF` when configuring a headless build.

The desktop camera uses left-drag to orbit, middle/right-drag to pan, the mouse wheel to zoom, and
`R` to reset. Simulation coordinates remain SI-valued doubles; rendering scale and apparent body
radius are visualization-only values.

The dockable engineering workspace provides a scene hierarchy, validated property inspector,
transport controls, global gravity/time settings, and live diagnostics. Scene and solver edits are
queued and applied at deterministic simulation boundaries; rejected numeric input appears in the
diagnostic command log.

Semi-Implicit Euler, Velocity Verlet, and classical RK4 are selectable at runtime. The diagnostics
and plots report mechanical energy and momentum errors relative to the initial sample, while the
timestep analyzer warns when orbital or pair-crossing timescales are under-resolved. Run
`aetherion_compare` for the reproducible one-period Earth-like integrator comparison.
