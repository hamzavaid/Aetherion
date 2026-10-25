# Aetherion user guide

[Back to the visual overview](README.md) · [Math and implementation techniques](TECHNICAL_GUIDE.md)

## First run

Launch `aetherion_app` from your build directory. Open **Simulation Controls** in the bottom
tab group; preset buttons are farther down that scrollable panel. Pick a preset before pressing
**Play**. Loading a preset replaces the current scene, so save anything you want to keep first.

The workspace has a scene hierarchy on the left, an inspector on the right, and dockable controls,
diagnostics, plots, probes, and save history. Drag tab headers to rearrange the workspace.

## Experiments to try

### Make two charges repel

1. Choose **Like charges** in Simulation Controls.
2. Set **Physics dt (s)** to `0.05` and **Time scale** to `10`.
3. Turn on **Object trails**, then press **Play**.
4. Both positive charges move away from one another. Pause and inspect their velocities.
5. Reset, change one charge from `1e-6` to `-1e-6 C`, and replay: the initial force reverses.

These slower, heavier demonstration particles need simulated time to move visibly. The preset's
very small default timestep/time scale is deliberately slow. Increasing time scale without an
appropriate timestep can hit the bounded catch-up limit rather than speeding up indefinitely.

There is no collision/contact solver: pause before an attractive pair reaches a close encounter.
Use **Single Step** to inspect behavior near a source guard rather than treating an overlap as a
physical bounce. Check **Diagnostics** for rejected operations and timestep warnings.

### Add a test charge to a fixed dipole

1. Choose **Electric dipole**. The two source bodies are fixed.
2. Click **Add Charged Body** in Scene Hierarchy, then select the new body.
3. Verify **Fixed** is off, mass is positive, charge is nonzero, and **Electrostatic interaction**
   is checked. Set its charge to `1e-6 C` and mass to `1 kg` for a simple test.
4. Set position to `[0, 0, 1] m`, enable velocity/force arrows, then step or play.
5. Try charge `-1e-6 C`, then `0 C`: the electric response reverses, then disappears.

Editing body state uses the simulation command queue. A new value takes effect at the next
command boundary, even while paused; invalid edits leave the original state intact.

### Compare field views

From **Electric dipole**, switch **Display mode** between **None**, **Observed Vector Field**,
**Field Lines**, and **Field Magnitude Plane**. Choose electric, magnetic, or gravity separately.

- **Vectors:** choose a 3D volume or XY/XZ/YZ slice, resolution, length, and normalized/log/linear
  scaling. Normalized arrows show direction, not equal physical strength.
- **Lines:** adjust automatic seeds, custom seed positions, trace step, maximum steps/length,
  and forward/backward tracing. Keep 2D field view off for three-dimensional lines.
- **Magnitude plane:** use **Fit plane to camera view** to follow zoom, or turn it off and enter
  a fixed center/half extent. The slice is double-sided, not a volumetric field solution.
- **Colors:** the color picker appears only for the active field's vector or line mode.

Field lines are tangents to a field, not particle trajectories. A charged particle can cross
magnetic field lines, and a moving particle need not follow an electric field line.

### Change a gyro orbit

1. Choose **Uniform-B gyro** and select the particle.
2. In the inspector, keep mass `1 kg`, charge `1 C`, initial velocity `[1, 0, 0] m/s`.
3. Under Simulation Controls, keep **Boris** and uniform B `[0, 0, 1] T`.
4. The ideal radius is `1 m` and the period is approximately `6.283 s` of simulation time.
5. Reset and double B to `[0, 0, 2] T`: ideal radius and period halve. Halve the timestep too
   when keeping the same number of steps per gyro period.

For an easy saved baseline, set these values, click **Save**, then run. **Reset** returns to
that checkpoint. A global reset without a checkpoint returns to the loaded preset instead.

In **Crossed E/B**, E is `[0, 1, 0] V/m`, B is `[0, 0, 1] T`, and the initial velocity is
`[1, 0, 0] m/s`: electric and magnetic forces cancel. Change the velocity to see deflection.

### Measure a field with a probe

Add a probe in **Field Probes**. Its position is fixed in world meters and it exerts no force.
Enable its E/B/gravity arrows; click it in the hierarchy or viewport to select it. **Locate in
scene** centers the camera without changing zoom. Hidden probe markers cannot be picked.

In **Plots**, choose **Probe |E|**, **Probe |B|**, or **Probe |g|** and the probe index.
For a single `1e-6 C` charge, an isolated probe `2 m` from its center measures about
`2246.888 V/m`, directed away from a positive charge. Remove other charges/uniform sources
when comparing against that single-source value.

For the README magnitude image: use a fixed dipole with charges `+1e-9` and `-1e-9 C`, an
XZ slice centered at the origin with half extent `[2.5, 1, 2.5] m`, resolution `32`, and a probe
at `[0, 0, 0.7] m`. Disable camera fitting for that explicit sampling region.

### Observe finite propagation delay

Choose **Propagation-delay probes**, press Play, and inspect the near/far probe readouts.
The source moves at `0.01c`, the timestep is `1 ns`, and probes are `3 m` and `6 m` from its
initial position. First valid samples arrive at about `11 ns` and `21 ns`.

Plot **Probe EM availability**, **Probe flight delay**, or **Probe earliest source time**.
An unavailable early sample means history is still warming up, not zero electric field.
Reset, trajectory edits, timestep changes, and scene loads restart that history.

The delayed-Coulomb display is an educational low-speed approximation. Particle forces remain
instantaneous; it is not a radiation or relativistic electrodynamics solver.

## Camera, selection, and movement

- **Focus Camera** frames the selected body once. **Follow Camera** keeps its center targeted
  while retaining your zoom/orbit; uncheck it to detach.
- **Follow center of mass camera** tracks the mass-weighted scene center instead of one object.
- **Show 2D field view** locks an orthographic XZ view and projects out-of-plane field components
  for display. It does **not** constrain three-dimensional particle dynamics.
- **Body radius multiplier** and **Minimum apparent radius** make small bodies visible without
  changing physical radii, mass, forces, or source guards. Picking uses the displayed radius.
- **Show engineering grid** toggles the grid; spacing automatically coarsens as you zoom out.

Select a body/probe, set **Move Selected → Move step (m)**, and use A/D, W/S, Q/E for world axes.
Arrow keys and Page Up/Down are alternatives. Holding a movement key repeats the step. Ctrl+Z
undoes one manual movement; Home or Ctrl+R resets to the current selection's movement baseline.
That reset is itself undoable. Undo does not reverse arbitrary property edits or simulated motion.
Body movement pauses playback. R alone resets the camera, not the simulation.

UI-owned mouse/keyboard input is not routed to camera or movement shortcuts.

## Saving and restoring

| Control | What it preserves | Lifetime |
| --- | --- | --- |
| Save | Physical scene, simulation time, runtime solver settings | Current session; up to 64 checkpoints |
| Reset | Restores the latest checkpoint, or loaded preset if none | Pauses simulation |
| Save History → Restore | Any retained physical checkpoint | Session only |
| Save Scene JSON | Body state, solver settings, visualization settings and probe definitions | `aetherion_scene.json` on disk |

Session checkpoints are not snapshots of every UI/display choice. Plot histories, camera pose,
source-history rings, and checkpoint logs are not stored in scene JSON. Loading JSON starts a
new simulation baseline at zero time; finite-delay probes warm up again.

JSON saves are validated before replacement. If an interrupted save leaves `.pending` or
`.previous` sidecars, preserve them before retrying; `.previous` may contain your last valid save.
The application will not silently overwrite an existing recovery sidecar.

## Build troubleshooting

**CMake not found:** install CMake and reopen the terminal so its `bin` directory is on PATH.
The preset also needs Ninja. A Visual Studio developer shell can provide MSVC; MSYS2/UCRT64
can provide GCC. Pick one compiler family for each build directory.

**GCC C compiler mixed with MSVC C++ compiler:** configure both compiler paths explicitly using
the [Windows commands in the README](README.md#quick-start). Use a new build directory.

**Cache contains another directory / WSL path:** do not reuse that cache. Configure into a new,
platform-specific directory such as `build/windows-gcc` or `build/linux-gcc`.

**Executable starts with missing DLL errors:** for UCRT64 builds, keep
`C:\msys64\ucrt64\bin` on PATH when launching, not just when compiling.

**No OpenGL 4.1 context:** check the graphics driver or remote-session GPU support. To run the
physics/tests without a desktop context:

```sh
cmake -S . -B build/headless -G Ninja -DAETHERION_BUILD_RENDERER=OFF
cmake --build build/headless
ctest --test-dir build/headless --output-on-failure
```

**Nothing seems to move:** check Pause/Play, time scale, fixed-body status, timestep/catch-up
limits, and whether the expected interaction is enabled globally and on each body. A neutral
body does not respond to E/B fields. Inspect Diagnostics for rejected edits or failed steps.

## Developer checks

```sh
ctest --preset dev
cmake --build build/dev --target format-check
```

Install `clang-format==23.1.1` before configuring to enable the format-check target. Compiler
warnings are errors by default. Optional clang-tidy is enabled with
`-DAETHERION_ENABLE_CLANG_TIDY=ON` when installed. Renderer-enabled tests include hidden-window
graphics/UI smoke checks; headless builds keep graphics-independent tests available.

Run `aetherion_headless` for the Earth-like orbit gate and `aetherion_compare` for the numerical
integrator comparison. See [the technical guide](TECHNICAL_GUIDE.md#diagnostics-and-validation)
for the error bounds and interpretation.
