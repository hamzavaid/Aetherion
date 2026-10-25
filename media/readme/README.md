# Demo media

These are real Aetherion OpenGL framebuffer captures, including Dear ImGui panels where
shown. They are not illustrations, desktop screenshots, or interpolated physics footage.
PNG preserves RGB samples; GIF uses a shared 256-color palette without dithering.

| Asset | Configuration |
| --- | --- |
| `workspace.png` | Electric dipole, 3D lines, 48 automatic seeds, display radius multiplier 2; 1440 × 900 |
| `electric-dipole.png` | Same dipole without panels; grid hidden; 960 × 600 |
| `electric-plane.png` | Fixed ±1 nC dipole, XZ magnitude slice, 32 × 32 samples, half extents (2.5, 1, 2.5) m, probe at (0, 0, 0.7) m; 1440 × 900 |
| `earth-moon.gif` | Earth–Moon, top-down camera, selected Moon, display radius multiplier 6, trail duration 2.5 × 10⁶ s; 960 × 600 |
| `magnetic-gyro.gif` | Uniform-B gyro, Boris, 4³ magnetic vectors, selected particle, velocity arrow and trail; 960 × 600 |

Both animations contain 100 frames at 20 fps and loop after five seconds. Gravity advances
79 fixed 300 s steps per frame; gyro advances two fixed 2π/200 s steps per frame. There are
100 warm-up frames to fill trails before recording. Playback is accelerated, not wall-clock
simulation speed. Slight seam differences reflect numerical motion rather than frame blending.
Display sizes, density and framing are deliberately adjusted for readability.

## Regenerate

The optional capture executable is disabled by default and requires the renderer, UI, and
a working OpenGL 4.1 driver, even though its window is hidden. It reads only built-in presets
and does not read saved UI layouts or capture other desktop windows. Pillow is only a
documentation-packaging dependency, not an application dependency.

From a configured development build:

```sh
cmake -S . -B build/dev -DAETHERION_BUILD_MEDIA_TOOLS=ON
cmake --build build/dev --target aetherion_capture
python -m pip install Pillow
```

Use a fresh output directory. On Windows add `.exe` to the executable:

```sh
build/dev/aetherion_capture workspace build/readme-frames/workspace
build/dev/aetherion_capture dipole build/readme-frames/dipole
build/dev/aetherion_capture magnitude build/readme-frames/magnitude
build/dev/aetherion_capture gravity build/readme-frames/gravity
build/dev/aetherion_capture gyro build/readme-frames/gyro
python tools/package_readme_media.py --frames build/readme-frames --overwrite
```

The raw PPM frames stay in the ignored build directory. The capture tool refuses to replace
existing frames. The packager requires explicit `--overwrite` before replacing media.
Different drivers/font rasterization may change pixels; this is not a pixel-golden test.

Verify committed assets without raw frames:

```sh
python tools/package_readme_media.py --verify
python -m unittest discover -s tools -p "test_*.py"
```

Verification decodes every frame and checks dimensions, frame counts, looping, 50 ms GIF
timing, actual animation movement, and a 4 MiB per-file limit.
