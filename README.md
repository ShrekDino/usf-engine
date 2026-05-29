# USF Engine — Unified Simplicial Framework for Godot 4.x

**Architecture:** Autopoietic simulation engine implementing the Palatini-Einstein-Cartan (PEC) framework within a discrete simplicial complex. Runtime complexity O(N·k) via Dynamic Coherence windowing.

## Module Registration

The engine registers 18 classes into Godot's ClassDB:

| Category | Class | Type | Parent |
|---|---|---|---|
| Constants | `USFConstants` | Resource | `Resource` |
| Core | `Simplex` | Struct | — (value type) |
| Core | `SimplicialComplex` | Resource | `Resource` |
| Core | `ReggeTensor` | Resource | `Resource` |
| Core | `LeeWickRegulator` | Resource | `Resource` |
| Core | `DynamicCoherence` | Resource | `Resource` |
| Physics | `Tetrad` | Resource | `Resource` |
| Physics | `TorsionField` | Resource | `Resource` |
| Physics | `PECSolver` | Node | `Node` |
| Physics | `BounceSolver` | Node | `Node` |
| Physics | `Soliton` | Resource | `Resource` |
| Physics | `HorizonMembrane` | Resource | `Resource` |
| Physics | `FormanRicci` | Resource | `Resource` |
| Thermo | `SzilardEngine` | Resource | `Resource` |
| Thermo | `GenerativeModel` | Resource | `Resource` |
| Thermo | `MarkovBlanket` | Resource | `Resource` |
| DQFR | `DQFRController` | Node | `Node` |
| World | `USFWorld` | Node3D | `Node3D` |

## 1. DQFR Duty Cycle

The `DQFRController` implements a two-phase stroboscopic duty cycle that governs temporal velocity and thermodynamic stability.

### Parameters

| Parameter | Symbol | Default | Range | Description |
|---|---|---|---|---|
| Drift duration | `Δt_drift` | 1.0 s | (0, ∞) | Blanket sealed, S_gen = 0 |
| Sample duration | `τ_sample` | 0.1 s | (0, ∞) | Blanket open, VFE descent |
| Ramp duration | `τ_ramp` | 0.02 s | (0, τ_sample) | Permeability rise time |
| Temporal velocity | `V_T` | computed | [0, 1] | V_T = 1 / (1 + Δt_drift / τ_sample) |
| DQFR ratio | `R_DQFR` | computed | [0, 1] | R_DQFR = Δt_drift / (Δt_drift + τ_sample) |

### Phase Transitions

**Drift Phase (`PHASE_DRIFT`):**
- `MarkovBlanket::seal()` — zero all sensory and active channels
- No computational work performed (S_gen = 0)
- `Ω_coherence` held at `Ω₀` base value
- Timer accumulates until `Δt_drift` elapsed

**Sampling Phase (`PHASE_SAMPLE`):**
- `MarkovBlanket::unshield()` — open to environmental flux
- `χ(t)` ramps permeability via raised cosine
- Active inference update: `q(s) ← (1-α)q(s) + α·p(s|o)`
- PEC gradient descent: `e^a_μ -= lr·grad·e^a_μ`
- Forman-Ricci metric damping: `ℓ_ij -= λ·RicF(ij)·ℓ_ij·dt`
- Ω_coherence update: `Ω = Ω₀·exp(-∫RicF·ds)`
- Lee-Wick Planck clamping: `ℓ_ij ≥ ℓ_Pl`

### Adiabatic Windowing

Blanket permeability follows a smooth raised cosine to prevent thermodynamic shock:

```
χ(t) = 0.5 · (1 - cos(π · t / τ_ramp))    for 0 ≤ t ≤ τ_ramp
χ(t) = 1.0                                 for t > τ_ramp
```

### Relativistic Anchoring

Lapse rate computed from local time-time metric component:

```
α_i = √(g₀₀(i))
Δt_local = Δt_coordinate · α_i
```

## 2. Geometric Primitives

### SimplicialComplex

4D hypercubic lattice generation with Minkowski signature (-,+,+,+).

**Constructor:**
```gdscript
var constants = USFConstants.new()
var lattice = SimplicialComplex.new()
lattice.from_hypercubic_lattice(n, spacing, constants)
```

**Properties:**
| Property | Type | Description |
|---|---|---|
| `vertices` | `Vector<Vec4d>` | 4D vertex positions [t, x, y, z] |
| `edges` | `Vector<Simplex>` | Edge simplices |
| `triangles` | `Vector<Simplex>` | Triangle hinges |
| `tetrahedra` | `Vector<Simplex>` | 3-simplices |
| `four_simplices` | `Vector<Simplex>` | 4-simplices |
| `edge_lengths` | `Vector<double>` | ℓ_ij ≥ ℓ_Pl |

**Minkowski metric:**
```
ds² = -Δt² + Δx² + Δy² + Δz²
```

### ReggeTensor

Discrete Einstein-Hilbert action from deficit angles at triangle hinges.

```
S_Regge = Σ_h ε_h · V_h / (16πG)
```

Where:
- `ε_h = 2π - Σθ_i` = deficit angle at hinge h
- `V_h` = hinge volume (triangle area)
- `θ_i` = dihedral angle between tetrahedra sharing hinge

Construction:
```gdscript
var regge = ReggeTensor.new()
regge.compute(lattice)
print("Regge action: ", regge.regge_action)
print("Avg curvature: ", regge.average_curvature())
```

### LeeWickRegulator

UV regulator enforcing Planck-length floor and regulated volume term.

**Volume term (from Eq. in the paper):**
```
V(√g) = -Φ₀ · [1 + (√g/√g₀)²]^{-α/2}
```

**Planck clamping:**
```gdscript
var lw = LeeWickRegulator.new()
lw.constants = constants
var clamped = lw.clamp_edge_length(raw_length)
var force = lw.lee_wick_force(edge_length)
```

## 3. PEC Physics Solver

### PECSolver

Iterative gradient descent on the combined Regge + torsion action. Operates as a `Node` — call `solve_step()` or `solve(max_iter, tolerance)`.

**Algorithm per step:**
1. Recompute `ReggeTensor` from current simplicial complex
2. Compute `TorsionField` from each simplex tetrad
3. Residual = `|S_Regge| + ⟨ρ_torsion⟩`
4. Gradient descent on tetrad frames: `e^a_μ -= lr · grad · e^a_μ`

```gdscript
var pec = PECSolver.new()
pec.constants = constants
pec.complex = lattice
add_child(pec)

var converged = pec.solve(100, 1e-6)
print("G_00: ", pec.einstein_tensor_component(0, 0))
print("T_00: ", pec.stress_energy_tensor_component(0, 0))
```

### BounceSolver

Implements the Raychaudhuri equation with torsion-driven repulsion.

**Modified Friedmann equations:**
```
H² = 8πG·ρ_m/3 + 3κ²S² - k/a²
ä/a = -4πG/3c² · (ρ+3p/c²) + 3κ²S²/a
```

**Bounce criterion:** `H → 0` at `a → a_min > ℓ_Pl`

```gdscript
var bounce = BounceSolver.new()
bounce.constants = constants
bounce.initialize(1.0, 1e-10, 1e-12, 0.0)
bounce.integrate(20000, 0.01)
print("Bounce: ", bounce.bounce_detected)
print("a_min: ", bounce.a_min)
print("Torsion dominated: ", bounce.torsion_dominated())
```

**Raychaudhuri equation (frame invariance):**
```
θ̇ = -θ²/3 - σ² + ω² - κ/2(ρ+3p) + 3κ²S_μS^μ
```
The torsion term `3κ²S_μS^μ` provides defocusing (`θ̇ > 0`) at microscopic scales without NEC violation.

### TorsionField

Connection torsion, contorsion, and axial vector extraction.

```
T^a_μν = ∂_μ e^a_ν - ∂_ν e^a_μ + ω_μ^{ab}e_{bν} - ω_ν^{ab}e_{bμ}
S_μ = (1/6) ε_{μνρσ} T^{νρσ}
ρ_torsion = 1.5 · κ² · s²  where s² = Σ T_{abc}²
```

**Braiding constraint:** `δS_braid/δω = 0` — torsion sourced solely by spin current.

## 4. Thermodynamic Metabolism

### SzilardEngine

Informational-to-work conversion with Landauer-bound accounting.

```
W = k_B · T · ln(2) · bits · η
η = 1 - T_cold / T_hot
E_min = k_B · T · ln(2) per bit erased
```

```gdscript
var engine = SzilardEngine.new()
engine.constants = constants
engine.initialize(1000.0, 300.0)
var work = engine.extract_work(50.0)
print("Efficiency: ", engine.efficiency)
print("Alive: ", engine.is_alive())
engine.step(delta)
```

### GenerativeModel

Active inference engine minimizing Variational Free Energy:

```
F = D_KL[q(s)||p(s)] - E_q[ln p(o|s)]
q(s) ∝ p(s) · Π_j p(o_j|s)^{o_j}
```

```gdscript
var model = GenerativeModel.new()
model.constants = constants
model.initialize(4, 3)
var obs = [0.6, 0.3, 0.1]
var vfe = model.variational_free_energy(obs)
model.update_beliefs(obs, 0.1)
print("Entropy: ", model.entropy())
print("Metabolic cost: ", model.metabolic_cost())
```

### MarkovBlanket

Statistical boundary partitioning state space into X = {μ, η, s, a}.

| Partition | Index | Role |
|---|---|---|
| μ (internal) | 0..n_μ | Autonomous self-model |
| s (sensory) | n_μ..n_μ+n_s | External→internal interface |
| a (active) | n_μ+n_s..n_μ+n_s+n_a | Internal→external interface |
| η (external) | remainder | Environmental flux |

```gdscript
var blanket = MarkovBlanket.new()
blanket.initialize(4, 3, 2, 4)

blanket.sense([0.8, 0.3, 0.1])
blanket.update_internal(0.1)
print("Vitality: ", blanket.vitality())
print("Permeable: ", blanket.is_permeable())
blanket.seal()
blanket.unshield([0.5, 0.5, 0.0])
```

## 5. Forman-Ricci Geometric Stability

### FormanRicci

Discrete curvature providing low-pass damping on the metric tensor.

**Forman-Ricci curvature for edge e:**
```
RicF(e) = w(e) · [ Σ_{f⊃e} w(f)/w(e) + Σ_{v∈e} w(v)/w(e) - Σ_{e'∼e} |w(e)-w(e')|/w(e) ]
```

**Weights:**
- `w(e) = 1/ℓ²` (edge)
- `w(f) = 1/Area(f)` (triangle face)
- `w(v) = 1` (vertex)

**Metric damping (Langevin):**
```
∂ℓ_ij/∂t = -λ · RicF(ij) · ℓ_ij + ξ(t)
ξ(t) ~ √(2k_B·T·λ·Δt) · N(0,1)
```

**Topological thermostat:**
```
γ(I) = γ₀ · [1 + (I₀/I)²]^{α/2}
```

## 6. Black Hole Soliton & Membrane

### Soliton

Regular black hole interior as a 3-form gauge field bubble.

```
p(r) = ρ_c·c² · (1 - r²/r_H²)      for r < r_H
p(r) = ρ_c·c² · exp(-(r-r_H)²/r_H²) for r ≥ r_H
```

```gdscript
var soliton = Soliton.new()
soliton.constants = constants
soliton.build(1.0e30, 1)
print("Compactness: ", soliton.compactness())
print("Topologically stable: ", soliton.topological_stability())
```

### HorizonMembrane

Israel junction conditions producing gravitational-wave echoes.

```
Δt = 4GM/c³ · ln(r_H / δ₀)
r_H = 2GM/c²
δ₀ = ℓ_Pl × F(Φ)
```

```gdscript
var bh = HorizonMembrane.new()
bh.constants = constants
bh.build(1.989e30, 0.0)  # solar mass
print("Echo delay: ", bh.echo_delay)
print("Frequency: ", bh.echo_frequency())
```

## 7. USFWorld — Systemic Root

Primary Godot interface node. Attach as root of SceneTree for full simulation.

```gdscript
var world = USFWorld.new()
add_child(world)
world.lattice_n = 4  # 4x4x4x4 grid
world.initialize_world()

# Monitor properties (read-only in Inspector)
print("VFE: ", world.variational_free_energy)
print("Ω: ", world.omega_coherence)
print("V_T: ", world.temporal_velocity)
print("a_min: ", world.bounce_a_min)
```

### GDScript API

| Property | Type | Access | Description |
|---|---|---|---|
| `lattice_n` | int | RW | Grid resolution (2-10) |
| `variational_free_energy` | float | R | Current VFE from GenerativeModel |
| `dqfr_ratio` | float | R | R_DQFR = Δt_drift / (Δt_drift + τ_sample) |
| `omega_coherence` | float | R | Current Ω_coherence |
| `temporal_velocity` | float | R | V_T (lapse-scaled) |
| `bounce_a_min` | float | R | Minimum scale factor |
| `metabolic_efficiency` | float | R | Carnot efficiency of SzilardEngine |
| `bounce_detected` | bool | R | Bounce trigger status |

## O(N·k) Complexity Proof

The engine enforces strict locality through the Dynamic Coherence Radius:

```
Interaction(i,j) = Interaction_raw(i,j) · exp(-d(i,j)² / Ω_coherence²)
```

Each vertex interacts only with vertices within its Ω_coherence neighborhood. The neighborhood size is bounded by:

```
k = ρ · V_d(Ω_coherence)
  = ρ · π^{d/2} / Γ(d/2 + 1) · Ω_coherence^d
```

where ρ is the vertex density and d is the local Hausdorff dimension. Since k is constant for fixed Ω_coherence and ρ, total complexity per iteration is O(N·k) = O(N). Communication cost per node remains bounded as N → 10⁶.

## Unit System

All internal computation uses natural units (ℏ = c = 1). The USFConstants Resource provides conversion to SI:

| Constant | Value | Unit |
|---|---|---|
| ℓ_Pl | 1.616×10⁻³⁵ | m |
| t_Pl | 5.391×10⁻⁴⁴ | s |
| M_Pl | 2.176×10⁻⁸ | kg |
| κ | √(8πG) = 4.09×10⁻⁵ | kg⁻¹/²·m¹/² |
| Λ (Lee-Wick) | M_Pl/100 | J |
| α | 1.0 | (tunable) |
| Φ₀ | ρ_Pl | J/m³ |
