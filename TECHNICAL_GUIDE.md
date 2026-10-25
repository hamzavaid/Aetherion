# Math and implementation techniques

This guide describes the implemented models, not a promise of accuracy for arbitrary scenes.
For experiments and controls, see the [user guide](GUIDE.md).

## Units and coordinates

Physics uses double-precision SI quantities: metres, seconds, kilograms, coulombs,
electric field in V/m, magnetic field in teslas, and magnetic dipole moment in A·m².
Positions and velocities belong to a right-handed, inertial Cartesian world frame.
Fixed bodies still act as sources but do not move. There is no collision/contact solver.

Rendering subtracts a nearby camera origin in double precision before conversion to GPU
floats. A metres-to-render-units factor supports local and astronomical scenes. Display
radius multipliers and minimum visible radii affect rendering and picking, not mass,
charge, separation, or dynamics. Origin subtraction cannot recover information already
lost when a world-coordinate displacement is below double precision's resolution.

## Force models

Let **d** = **r**ⱼ − **r**ᵢ, and R = √(‖**d**‖² + ε²), where ε is the configured
softening length in metres. The direct pair solver is O(N²):

```text
gravity:      a_i = Σ_j G m_j d / R³
electrostatic F_i = Σ_j −k q_i q_j d / R³
electric E(r)     = Σ_j k q_j (r − r_j) / (|r − r_j|² + ε²)^(3/2)
Lorentz F         = q [E + v × B]
```

G = 6.67430 × 10⁻¹¹ m³ kg⁻¹ s⁻² and k = 8.9875517923 × 10⁹ N m² C⁻².
Positive charges repel; opposite charges attract. A positive particle travelling along
+X in a +Z magnetic field accelerates toward −Y. Zero charge gives no electromagnetic
force. Electric forces and arbitrary-point samples share the point-charge kernel.
External uniform electric fields and magnetic sources superpose with supported sources.

Near-source guards and softening are numerical policies, not collision physics. Guarded
pair interactions are skipped; guarded field samples are invalid and are not drawn.
Gravity visualization also excludes source interiors. A finite physical radius is not
an extended-density force model. Zero fields are handled without normalizing zero vectors;
invalid/nonfinite results are rejected. Products are arranged around normalized directions
and inverse-square magnitudes to reduce avoidable overflow.

Magnetic sources include uniform fields and static ideal dipoles. Outside the configured
source guard, the dipole model is

```text
B(r) = μ₀/(4π |r|³) [3(m · r_hat) r_hat − m]
μ₀ = 1.25663706212 × 10⁻⁶ N/A²
```

Here **r** is measured from the dipole centre. This is not a resolved magnet geometry.
There are no magnetic monopoles and no general self-consistent moving-charge magnetic solver.
Dynamics are nonrelativistic; high speeds do not introduce relativistic inertia.

## Time stepping and integrators

The accumulator advances fixed simulation steps h, independently of render-frame duration.
Time scale changes how many simulation seconds are requested, not h. A maximum substep
budget prevents a slow frame from locking the application; excess catch-up time is dropped
and reported. Rejected steps do not advance accepted simulation time.

| Integrator | Update or technique | Appropriate use and caveats |
| --- | --- | --- |
| Semi-Implicit Euler | vₙ₊₁ = vₙ + h aₙ; rₙ₊₁ = rₙ + h vₙ₊₁ | First order; symplectic for suitable conservative position-dependent forces |
| Velocity Verlet | Half kick, full drift, recompute acceleration, half kick | Second order for position-dependent acceleration; not generally a symplectic Lorentz solver |
| RK4 | Four stages of the coupled state derivative (v, a), weights 1:2:2:1 | Fourth order for smooth ODEs; neither symplectic nor exactly energy preserving |
| Boris | Electric/nonmagnetic half kicks around a magnetic rotation | Preferred for resolved charged-particle gyro motion; see the exact implementation below |

For Boris, let a_ng be acceleration excluding the magnetic term:

```text
v_minus = v_n + (h/2) a_ng(r_n)
t       = q h B(r_n)/(2m)
s       = 2t/(1 + t·t)
v_prime = v_minus + v_minus × t
v_plus  = v_minus + v_prime × s
r_new   = r_n + h v_plus
v_new   = v_plus + (h/2) a_ng(r_new)
```

This documents the actual kick/rotation/drift implementation: it does not include an
unwritten half-position drift. The Cayley rotation preserves speed in a uniform pure
magnetic field up to roundoff. That does not eliminate position/phase error or establish
general second-order accuracy for arbitrary spatially varying magnetic fields. Resolve
the gyro period and compare smaller timesteps for the scene being studied.

For a uniform field, ω_c = |q|B/m, T = 2π/ω_c, and ρ = v_perpendicular/ω_c.
The unit gyro preset has m = 1 kg, q = 1 C, B = 1 T and speed 1 m/s:
ρ = 1 m and T ≈ 6.283185 s. Parallel velocity produces a helix. For crossed uniform
fields, E × B / B² is the ideal nonrelativistic guiding-centre drift.

Steps, ordering, and command application are deterministic for the same configuration
and executable. Floating-point results are not guaranteed bitwise identical across
compilers, architectures, or optimization settings.

## Fields and scientific visualization

The renderer consumes arbitrary-position field providers; it does not implement a second
force solver. Sampling/tracing data can be tested without a graphics context.

**Vector fields** sample 3D lattices or XY/XZ/YZ slices. Linear, logarithmic and normalized
length mappings make large dynamic ranges usable. Magnitude thresholds filter samples;
zero, singular and invalid vectors are omitted. Glyph lengths and colors are presentation
settings, not changes to field strength.

**Field lines** use a separate RK4 tracer for the normalized tangent equation

```text
dr/ds = ±F(r)/|F(r)|
```

The step is an arc-length-like distance in metres, not a simulation timestep. Forward and
backward tracing share configured length/work budgets. Traces stop at source guards,
weak/invalid samples, region boundaries, stagnation, or length/step limits. Global work
bounds prevent unbounded tracing. Cached results are invalidated by source or settings
changes; moving sources can require rebuilding every update. Field lines are instantaneous
direction curves, not particle trajectories or conserved-energy contours.

**Magnitude planes** color sampled |F| using a normalized log1p mapping over valid plane
samples. This is a display mapping in the chosen field's SI units, not electric potential,
energy density, or a volume reconstruction. Equal finite magnitudes receive the same
intensity. Camera-fitting can enlarge the plane; resolution still limits spatial detail.

**Probes** sample at fixed world positions without adding sources or forces. Their arrows
and plots use the same provider. A 2D field view projects presentation onto a chosen slice;
it does not constrain the underlying 3D particle dynamics.

## Finite-delay preview

The propagation preview solves the retarded-time equation using bounded source histories:

```text
t_r = t − |r − r_source(t_r)|/c,     c = 299792458 m/s
```

Position/velocity history is linearly interpolated, with no extrapolation. Fixed-point
iteration has a default limit of 64 iterations and a time tolerance combining 10⁻¹⁵ s,
10⁻¹⁰ times the delay, and floating-point time resolution. Missing history, nonconvergence,
singular samples, and sampled luminal/superluminal motion invalidate the result. Storage
is bounded: up to 65,536 samples per source, 256 sources and 1,048,576 samples overall.

The sampled field is a **delayed Coulomb preview**, not the full Liénard–Wiechert solution:
it omits velocity/radiation terms and induced magnetic fields. Particle forces remain
instantaneous. Use it for low-speed propagation demonstrations, not radiation predictions.
Reset, loading, and trajectory edits require history warm-up again; histories are not
stored in scene JSON. The delay preset's 3 m/6 m probes first become valid on the 11 ns/
21 ns accepted steps, illustrating finite sampling as well as light-travel time.

## Diagnostics and validation

Kinetic energy is Σ ½m|v|²; gravitational pair potential is −Gmᵢmⱼ/r for the runtime
Newtonian model. Electrostatic pair potential uses kqᵢqⱼ/R consistently with electric
softening; a uniform external electric field contributes −ΣqᵢE·rᵢ. Magnetic force does
no work in the continuous model. Momentum is Σmv and angular momentum is Σr×mv.
Fixed sources, external fields, edits and guarded interactions can make closed-system
conservation expectations inappropriate.

Energy drift is |H − H₀|/|H₀| when H₀ is nonzero. A zero baseline reports zero if unchanged
and one otherwise, rather than dividing by zero. Momentum drift is an absolute vector
difference norm. Baselines restart after relevant scene/model changes. Reference-frame
readouts translate position and velocity; they do not introduce rotating-frame dynamics.

Representative automated checks include:

- Earth-like Euler orbit over one year at h = 3600 s: radius error below 0.2% and relative
  energy drift below 2 × 10⁻⁵.
- Analytical gravity, Coulomb and Lorentz directions/magnitudes, superposition, zero-charge
  and singularity policies, plus field-line direction and safe termination.
- Uniform-field Boris over 100 gyro periods, 200 steps per period: speed drift below
  2 × 10⁻¹² and orbit closure error below 0.06 m in the unit test configuration.
- Integrator comparison and RK4 refinement tests. A short smooth-orbit accuracy ranking
  is not a universal long-term stability guarantee.
- Serialization, command queues, reset/checkpoints, camera precision, picking, visualization
  data, and hidden-context renderer/UI regressions.

Run `ctest --preset dev`, `build/dev/aetherion_headless`, and
`build/dev/aetherion_compare` (add `.exe` on Windows). Test definitions live
in [tests](tests); implementation is in [src](src) and [include](include).

## Architecture and scope

Core scene/state and FIFO commands are separate from force providers, particle integrators,
visualization data generation, OpenGL presentation, and Dear ImGui controls. Physical edits
are applied at simulation boundaries; failed state updates are rolled back. Scene JSON
persists supported physical/presentation settings, not every transient UI or history buffer.
See [saving behavior](GUIDE.md#saving-and-restoring).

The direct solvers prioritize inspectability over large-N scaling. There is no Barnes–Hut,
parallel/GPU physics, relativistic dynamics, general radiation solver, FDTD or PIC model.
Always test timestep refinement and model validity before interpreting a scene quantitatively.
