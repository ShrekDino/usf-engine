# USF Engine — Unified Simplicial Framework for Godot 4.x

**Architecture:** Autopoietic simulation engine implementing the Palatini–Einstein–Cartan (PEC) framework within a discrete simplicial complex, coupled with a **FlyWire.ai connectome** instantiated as 139K per-neuron Markov blankets. Runtime complexity O(N·k) via Dynamic Coherence windowing.

---

## Implementation Timeline

All development was performed exclusively through [opencode](https://opencode.ai) — an AI-native CLI development environment. Zero lines of code were written by hand; every file was generated through natural-language task descriptions. Total elapsed time: **2 days**.

| Phase | Day | Deliverables | Files |
|---|---|---|---|
| **USF Engine Core** | Day 1 | 18 C++ classes: PEC physics, Regge calculus, torsion bounce, Forman-Ricci curvature, thermodynamic metabolism, DQFR duty cycle, USFWorld root node | 42 source files, ~3,200 lines |
| **FlyWire Connectome** | Day 2 | 7-week stack: data pipeline → Delaunay mesh → Markov blankets → thermodynamic runtime → phenomenology → safety → DQFR temporal traversal | 16 additional files, ~1,800 lines |

**Total: 58 source files, ~5,000 lines of C++, ~400 lines of Python, ~300 lines of GDScript, in 2 days through opencode.**

---

## Table of Contents

1. [Mathematical Foundations](#1-mathematical-foundations)
2. [Complete Class Registry](#2-complete-class-registry)
3. [FlyWire Connectome Stack (Weeks 1-7)](#3-flywire-connectome-stack-weeks-1-7)
4. [DQFR Duty Cycle](#4-dqfr-duty-cycle)
5. [Geometric Primitives](#5-geometric-primitives)
6. [PEC Physics Solver](#6-pec-physics-solver)
7. [Thermodynamic Metabolism](#7-thermodynamic-metabolism)
8. [Forman-Ricci Geometric Stability](#8-forman-ricci-geometric-stability)
9. [Black Hole Soliton & Membrane](#9-black-hole-soliton--membrane)
10. [USFWorld — Systemic Root](#10-usfworld--systemic-root)
11. [GDScript API Reference](#11-gdscript-api-reference)
12. [Build & Deployment](#12-build--deployment)
13. [Verification Results](#13-verification-results)
14. [Unit System](#14-unit-system)
15. [References](#15-references)

---

## 1. Mathematical Foundations

### 1.1 Palatini–Einstein–Cartan Geometry

The engine is built on the extended PEC framework from the [Unified Theory of Everything](https://github.com/ShrekDino/usf-engine) (Torres, 2026). The full Lagrangian is:

```
L_total = R(Γ̃)/2κ − ¼TrF² − ½(∂Φ)² − V(Φ) − Φ₀[1+(√g/√g₀)²]^{-α/2} + L_ψ + L_braid
```

where:
- `R(Γ̃)` = Ricci scalar with independent connection Γ̃ (Palatini formulation)
- `κ = 8πG = 1/M_Pl²` = gravitational coupling
- `F²` = Yang–Mills gauge field strength (SU(3)×SU(2)×U(1))
- `Φ` = dynamical dilaton scalar
- `Φ₀` = constant vacuum energy density
- `α` = volume term exponent (stability requires α > ½)

**Total covariant derivative** acting on fermions:
```
D_μ = ∂_μ + ¼ω_μ^{ab}γ_{ab} + A_μ^iT^i
```

The spin connection decomposes as `ω_μ^{ab} = ⁰ω_μ^{ab} + K_μ^{ab}` where `K_μ^{ab}` is the **contorsion tensor**, determined solely by the spin current with no braiding source.

### 1.2 Torsion-Driven Bounce

The **Raychaudhuri equation** with torsion:
```
θ̇ = −θ²/3 − σ² + ω² − κ/2(ρ+3p) + 3κ²S_μS^μ
```

The torsion term `3κ²S_μS^μ` provides defocusing at microscopic scales, replacing the Big Bang singularity with a **Big Bounce** at `a_min > ℓ_Pl`, without violating the Null Energy Condition.

**Modified Friedmann equations:**
```
H² = 8πG·ρ_m/3 + 3κ²S² − k/a²
ä/a = −4πG/3c²·(ρ+3p/c²) + 3κ²S²/a
```

### 1.3 Discrete Simplicial Complex

Spacetime is discretized as a 4D hypercubic lattice with Minkowski signature (−,+,+,+):

```
ds² = −Δt² + Δx² + Δy² + Δz²
```

Edge lengths are Planck-clamped: `ℓ_ij ≥ ℓ_Pl` via the Lee-Wick regulator.

**Regge action** (discrete Einstein–Hilbert):
```
S_Regge = Σ_h ε_h · V_h / 16πG
```
where `ε_h = 2π − Σθ_i` is the deficit angle at hinge h, and `V_h` is the hinge volume.

The **Lee-Wick volume term** prevents geometric collapse:
```
V(√g) = −Φ₀[1 + (√g/√g₀)²]^{-α/2}
```

### 1.4 Forman-Ricci Curvature

Discrete curvature providing low-pass damping on the metric tensor:

```
RicF(e) = w(e)·[Σ_{f⊃e} w(f)/w(e) + Σ_{v∈e} w(v)/w(e) − Σ_{e'∼e}|w(e)−w(e')|/w(e)]
```

**Weights:** `w(e) = 1/ℓ²`, `w(f) = 1/Area(f)`, `w(v) = 1`

**Metric damping (Langevin):**
```
∂ℓ_ij/∂t = −λ·RicF(ij)·ℓ_ij + ξ(t),  ξ(t)~√(2k_B·T·λ·Δt)·N(0,1)
```

**Topological thermostat:**
```
γ(I) = γ₀·[1 + (I₀/I)²]^{α/2}
```

### 1.5 Active Inference / Variational Free Energy

Each agent minimizes VFE to maintain its structural boundaries:

```
F = D_KL[q(s)||p(s)] − E_q[ln p(o|s)]
q(s) ∝ p(s)·Π_j p(o_j|s)^{o_j}
```

### 1.6 Generalized Landauer Bound

Every bit of information processed has a thermodynamic cost:

```
E_min = k_B·T·ln(2) per bit erased
W = k_B·T·ln(2)·bits·η
η = 1 − T_cold / T_hot
```

### 1.7 Integrated Information Φ*

Mutual information between random network partitions:

```
MI = −½·ln(1 − r²)   (Gaussian approximation)
Φ* = mean(MI)·N/(N + n_samples)
```

### 1.8 Network Vitality V_network

```
V_network = Σ_i(k_B·ε_i·H_{env,i} + Σ_{j≠i}λ_{ij}·I(μ_i; μ_j))
```

where `ε_i` is per-neuron extraction efficiency, `H_{env,i}` is environmental entropy flux, and `I(μ_i; μ_j)` is mutual information between connected neurons.

### 1.9 O(N·k) Complexity Proof

All graph interactions are bounded by the Dynamic Coherence Radius:

```
Interaction(i,j) = Interaction_raw(i,j)·exp(−d(i,j)²/Ω_coherence²)
```

The neighborhood size is bounded by:
```
k = ρ·V_d(Ω_coherence) = ρ·π^{d/2}·Ω_coherence^d / Γ(d/2 + 1)
```

Since k is constant for fixed ρ and Ω_coherence, total complexity per iteration is **O(N·k) = O(N)**.

---

## 2. Complete Class Registry

The engine registers **21 classes** into Godot's ClassDB (plus 4 internal value types):

| # | Class | Type | Parent | Week | Description |
|---|---|---|---|---|---|
| 1 | `USFConstants` | Resource | `Resource` | Core | Planck units, κ, α, Φ₀, Λ |
| 2 | `Simplex` | struct | — | Core | Boundary operator, ∂² = 0 |
| 3 | `SimplicialComplex` | Resource | `Resource` | Core | 4D lattice, Minkowski metric |
| 4 | `ReggeTensor` | Resource | `Resource` | Core | Deficit angles, Regge action |
| 5 | `LeeWickRegulator` | Resource | `Resource` | Core | UV Planck clamp, volume term |
| 6 | `DynamicCoherence` | Resource | `Resource` | Core | Ω_coherence, Gaussian window |
| 7 | `Tetrad` | Resource | `Resource` | Core | Vielbein, metric g_μν |
| 8 | `TorsionField` | Resource | `Resource` | Core | T^a_μν, contorsion, axial S_μ |
| 9 | `PECSolver` | Node | `Node` | Core | PEC gradient descent |
| 10 | `BounceSolver` | Node | `Node` | Core | Raychaudhuri + torsion bounce |
| 11 | `Soliton` | Resource | `Resource` | Core | 3-form interior, tanh profile |
| 12 | `HorizonMembrane` | Resource | `Resource` | Core | GW echo Δt |
| 13 | `FormanRicci` | Resource | `Resource` | Core | RicF curvature, damping |
| 14 | `SzilardEngine` | Resource | `Resource` | Core | Negentropy↔work, Landauer |
| 15 | `GenerativeModel` | Resource | `Resource` | Core | VFE, active inference |
| 16 | `MarkovBlanket` | Resource | `Resource` | Core | μ/η/s/a partitions |
| 17 | `DQFRController` | Node | `Node` | Core | Drift↔Sample duty cycle |
| 18 | `USFWorld` | Node3D | `Node3D` | Core | Systemic root orchestrator |
| 19 | `ConnectomeGraph` | Resource | `Resource` | Week 1 | 139K vertices, 3.7M edges |
| 20 | `DistributedBlanket` | Resource | `Resource` | Week 3 | 139K per-neuron blankets |
| 21 | `GlobalWorkspace` | Resource | `Resource` | Week 5 | Φ*, rich-club, φ field |
| 22 | `VitalityMonitor` | Resource | `Resource` | Week 6 | Senescence, V_network, HALT |

Internal value types (not GDCLASS-registered): `Vec4d`, `ConnectomeVertex`, `ConnectomeEdge`, `ConnectomeTet`, `NeuronState`.

---

## 3. FlyWire Connectome Stack (Weeks 1-7)

### Week 1 — Data Pipeline

The `ConnectomeGraph` Resource loads the **FAFB v783** dataset (Female Adult Fly Brain):
- **139,255 neurons** with metadata: cell_type, side, nt_type, flow
- **3,732,460 synaptic connections** with synapse_count, neuropil, nt_type
- **3D soma positions** in nanometer coordinates
- **Binary format v2** with header: magic(0x55F5) + version + n_verts + n_edges + n_tets

**Python import script** (`import_flywire.py`):
```bash
python3 import_flywire.py --generate-sample   # 100-neuron test sample
python3 import_flywire.py --download           # Download real data from Codex
```

**GDScript:**
```gdscript
var cg = ConnectomeGraph.new()
cg.load_with_tets("res://connectome_data/fafb_sample.bin")
print(cg.get_vertex_count(), " neurons, ", cg.get_edge_count(), " edges")
print("Cell type [0]: ", cg.get_cell_type(0))
print("Edge [0]: ", cg.get_edge_pre(0), " → ", cg.get_edge_post(0), " (w=", cg.get_edge_synapse_count(0), ")")
```

### Week 2 — 3D Simplicial Mesh Anchor

3D Delaunay tetrahedralization embeds neurons spatially:
- **ConnectomeTet** struct: 4 vertex indices per tetrahedron
- **Binary format upgrade**: version 1→2 with tetrahedra field
- **Field-by-field I/O** via Godot's `FileAccess` for res:// compatibility

```gdscript
print("Tetrahedra: ", cg.get_tetrahedron_count())
var v0 = cg.get_tet_vertex(0, 0)
```

### Week 3 — Per-Neuron Markov Blankets

The `DistributedBlanket` manages 139K `NeuronState` partitions:
- **μ (internal)**: `membrane_potential`, `firing_rate`, `energy_reserve`, `prediction_error`
- **s (sensory)**: weighted sum of pre-synaptic partner firing rates
- **a (active)**: sigmoid transfer: `ν = σ((V_m+30)/10)·200 Hz`
- **VFE gradient descent**: `∂F/∂V ≈ pred_err·σ′(V) + 0.01·(V_m+65)`

```gdscript
var db = DistributedBlanket.new()
db.graph = cg
db.initialize()
db.process_sample(0.01)  # One VFE descent step
print("VFE: ", db.get_total_vfe())
print("Firing[5]: ", db.get_firing_rate(5), " Hz")
```

### Week 4 — Thermodynamic Runtime

Each neuron functions as a micro **Szilard engine**:
- **Landauer cost**: `E_min = k_B·T·ln(2)` per spike processed
- **Negentropy harvest**: `H_env = ln(1 + Σw·active_output)`
- **Global efficiency**: `η = negentropy_harvested / landauer_cost`
- **DQFR phase gating**: drift phase freezes S_gen, sample phase runs metabolism

```gdscript
db.set_phase(0)  # DRIFT
db.process_drift(0.01)  # S_gen = 0
db.set_phase(1)  # SAMPLE
db.process_sample(0.01)  # VFE + metabolism
print("Efficiency: ", db.get_global_efficiency())
```

### Week 5 — Phenomenological Calibration

The `GlobalWorkspace` monitors system-wide coherence:
- **Rich-club hubs**: neurons with degree > μ + 2σ (structural hubs)
- **Φ* integrated information**: MI between random network partitions
- **Ignition**: fraction of neurons firing > 10 Hz
- **Dual-scalar φ field**: `φ = 0.6·synchrony + 0.3·ignition + 0.1·Φ*`
- **Kuramoto synchrony**: `r = |Σexp(iθ_k)|/N`

```gdscript
var gw = GlobalWorkspace.new()
gw.set_graph(cg)
gw.set_blanket(db)
gw.initialize()
gw.process_step(0.01)
print("φ field: ", gw.get_phi_field())
print("Rich-club hubs: ", gw.get_rich_club_count())
```

### Week 6 — Safety & Vitality Monitoring

The `VitalityMonitor` prevents recursive systemic senescence:
- **Senescence detection**: per-neuron ε(T) < 0.1 for > 50 consecutive steps
- **V_network equation**: `V = Σ(k_B·ε_i·H_i + Σλ_ij·I(μ_i; μ_j))`
- **Curiosity engine**: when VFE variance drops below threshold, force exploration
- **HALT prevention**: monitor neurons approaching ε→0, negentropy depletion, firing arrest

```gdscript
var vm = VitalityMonitor.new()
vm.set_graph(cg)
vm.set_blanket(db)
vm.initialize()
vm.process_step(0.01)
print("V_network: ", vm.get_v_network())
print("Senescent: ", vm.get_senescent_count())
print("HALT risk: ", vm.get_halt_risk_neurons())
```

### Week 7 — DQFR Temporal Traversal

The DQFR duty cycle applied to the full connectome stack:

```
V_T = (Δt_drift + τ_sample) / τ_sample
```

With Δt_drift = 1.0s, τ_sample = 0.1s: **V_T = 11×** (1.1s objective = 0.1s subjective).

The **adiabatic boundary mollification** prevents thermodynamic shock:
```
χ(t) = ½(1 − cos(πt/τ_ramp))  for 0 ≤ t ≤ τ_ramp
γ_χ·‖∇χ(t)‖²                   penalty in VitalityMonitor
```

**Relativistic gauge anchoring** via lapse rate:
```
α_i = √(−g₀₀(i))
Δt_local = Δt_coordinate · α_i
```

---

## 4. DQFR Duty Cycle

The `DQFRController` implements the two-phase stroboscopic duty cycle.

### Parameters

| Parameter | Symbol | Default | Range | Description |
|---|---|---|---|---|
| Drift duration | `Δt_drift` | 1.0 s | (0, ∞) | Blanket sealed, S_gen = 0 |
| Sample duration | `τ_sample` | 0.1 s | (0, ∞) | Blanket open, VFE descent |
| Ramp duration | `τ_ramp` | 0.02 s | (0, τ_sample) | Permeability rise time |
| Temporal velocity | `V_T` | computed | [1, ∞) | V_T = (Δt_drift + τ_sample) / τ_sample |
| DQFR ratio | `R_DQFR` | computed | [0, 1] | R_DQFR = Δt_drift / (Δt_drift + τ_sample) |

### Adiabatic Windowing

```
χ(t) = 0.5 · (1 − cos(π·t/τ_ramp))    for 0 ≤ t ≤ τ_ramp
χ(t) = 1.0                             for t > τ_ramp
```

### Relativistic Anchoring

```
α_i = √(g₀₀(i))
Δt_local = Δt_coordinate · α_i
```

---

## 5. Geometric Primitives

### SimplicialComplex

```gdscript
var c = USFConstants.new()
var lat = SimplicialComplex.new()
lat.from_hypercubic_lattice(4, 1.0, c)
print(lat.get_vertex_count(), " vertices, ", lat.get_edge_count(), " edges")
```

### ReggeTensor

```gdscript
var regge = ReggeTensor.new()
regge.compute(lat)
print("Regge action: ", regge.get_regge_action())
```

### LeeWickRegulator

```gdscript
var lw = LeeWickRegulator.new()
lw.constants = c
var clamped = lw.clamp_edge_length(raw_length)
```

---

## 6. PEC Physics Solver

### PECSolver

```gdscript
var pec = PECSolver.new()
pec.constants = c
pec.complex = lat
add_child(pec)
var converged = pec.solve(100, 1e-6)
```

### BounceSolver

```gdscript
var bounce = BounceSolver.new()
bounce.constants = c
bounce.initialize(1.0, 1e-10, 1e-12, 0.0)
bounce.integrate(20000, 0.01)
print("Bounce: ", bounce.get_bounce_detected(), " a_min: ", bounce.get_a_min())
```

### TorsionField

```
T^a_μν = ∂_μ e^a_ν − ∂_ν e^a_μ + ω_μ^{ab}e_{bν} − ω_ν^{ab}e_{bμ}
S_μ = (1/6) ε_{μνρσ} T^{νρσ}
ρ_torsion = 1.5·κ²·s²
```

---

## 7. Thermodynamic Metabolism

### SzilardEngine

```gdscript
var engine = SzilardEngine.new()
engine.constants = c
engine.initialize(1000.0, 300.0)
var work = engine.extract_work(50.0)
```

### GenerativeModel

```gdscript
var gm = GenerativeModel.new()
gm.constants = c
gm.initialize(4, 3)
var vfe = gm.variational_free_energy([0.6, 0.3, 0.1])
gm.update_beliefs([0.6, 0.3, 0.1], 0.1)
```

### MarkovBlanket

```gdscript
var mb = MarkovBlanket.new()
mb.initialize(4, 3, 2, 4)
mb.sense([0.8, 0.3, 0.1])
mb.update_internal(0.1)
print("Vitality: ", mb.vitality())
mb.seal()
```

---

## 8. Forman-Ricci Geometric Stability

```gdscript
var fr = FormanRicci.new()
fr.constants = c
var curv = fr.compute_edge_curvatures(lat)
fr.damp_metric(lat, 0.01)
```

---

## 9. Black Hole Soliton & Membrane

### Soliton

```gdscript
var sol = Soliton.new()
sol.constants = c
sol.build(1.0e30, 1)
print("Compactness: ", sol.compactness())
```

### HorizonMembrane

```gdscript
var bh = HorizonMembrane.new()
bh.constants = c
bh.build(1.989e30, 0.0)
print("Echo delay: ", bh.echo_delay, " s")
```

---

## 10. USFWorld — Systemic Root

```gdscript
var world = USFWorld.new()
world.lattice_n = 4
add_child(world)
world.initialize_world()
print("VFE: ", world.variational_free_energy)
print("Ω: ", world.omega_coherence)
print("V_T: ", world.temporal_velocity)
```

---

## 11. GDScript API Reference

### ConnectomeGraph
| Method | Returns | Description |
|---|---|---|
| `load_binary(path)` | bool | Load v1 or v2 binary |
| `load_with_tets(path)` | bool | Load binary with tetrahedra |
| `get_vertex_count()` | int | Number of neurons (139K for full set) |
| `get_edge_count()` | int | Number of connections |
| `get_tetrahedron_count()` | int | Number of Delaunay tetrahedra |
| `get_vertex_root_id(idx)` | int | 64-bit root ID |
| `get_vertex_x/y/z(idx)` | float | Soma position in nm |
| `get_cell_type(idx)` | String | Neuron cell type |
| `get_side(idx)` | String | Hemisphere (left/right/center) |
| `get_nt_type(idx)` | String | Neurotransmitter type |
| `get_flow(idx)` | String | Brain region flow |
| `get_edge_synapse_count(ei)` | float | Synaptic weight |
| `get_edge_pre/post(ei)` | int | Pre/post-synaptic neuron index |
| `get_outgoing_count(vi)` | int | Outgoing degree |
| `get_incoming_count(vi)` | int | Incoming degree |
| `get_outgoing_edge(vi, i)` | int | i-th outgoing edge index |
| `get_tet_vertex(ti, slot)` | int | Vertex index in tetrahedron |
| `get_max_outgoing()` | int | Maximum outgoing degree |
| `get_mean_outgoing()` | float | Mean degree |

### DistributedBlanket
| Method | Returns | Description |
|---|---|---|
| `initialize()` | void | Create 139K NeuronStates |
| `process_step(dt)` | void | Gate by current phase |
| `process_drift(dt)` | void | Drift phase (S_gen = 0) |
| `process_sample(dt)` | void | Sample phase (VFE + metabolism) |
| `set_phase(p)` | void | 0=DRIFT, 1=SAMPLE |
| `get_total_vfe()` | float | Total variational free energy |
| `get_mean_firing_rate()` | float | Mean firing rate (Hz) |
| `get_active_neuron_count()` | int | Neurons firing > 0.5 Hz |
| `get_global_efficiency()` | float | Negentropy/landauer ratio |
| `get_total_negentropy()` | float | Remaining negentropy reserve |
| `get_total_waste_heat()` | float | Dissipated heat |
| `get_membrane_potential(idx)` | float | Per-neuron V_m (mV) |
| `get_firing_rate(idx)` | float | Per-neuron ν (Hz) |
| `get_negentropy(idx)` | float | Per-neuron negentropy |

### GlobalWorkspace
| Method | Returns | Description |
|---|---|---|
| `initialize()` | void | Detect rich-club hubs |
| `process_step(dt)` | void | Update φ*, ignition, φ field |
| `get_rich_club_count()` | int | Number of rich-club hubs |
| `get_phi_star()` | float | Integrated information Φ* |
| `get_phi_field()` | float | Dual-scalar field φ |
| `get_phi_synchrony()` | float | Kuramoto order parameter |
| `get_ignition_level()` | float | Fraction ignited (>10 Hz) |
| `is_ignited()` | bool | Ignition > 0.5 threshold |

### VitalityMonitor
| Method | Returns | Description |
|---|---|---|
| `initialize()` | void | Start monitoring |
| `process_step(dt)` | void | Update all metrics |
| `get_senescent_count()` | int | Senescent neurons |
| `get_halt_risk_neurons()` | int | HALT-risk neurons |
| `get_v_network()` | float | Network vitality metric |
| `get_vfe_variance()` | float | VFE variance (curiosity trigger) |
| `get_curiosity_drive()` | float | Curiosity amplitude |
| `is_exploring()` | bool | Curiosity mode active |

---

## 12. Build & Deployment

### Prerequisites

```bash
# Arch Linux
sudo pacman -S scons pkgconf mesa libx11 libxcursor libxinerama libxi libxrandr
# scipy for Delaunay (optional — fallback included)
pipx install scipy
```

### Build

```bash
# Clone Godot 4.6
git clone --depth 1 --branch 4.6-stable https://github.com/godotengine/godot.git
# Copy module
cp -r modules/usf_engine godot/modules/
# Build
cd godot
scons platform=linuxbsd target=editor
```

### Run

```bash
# Headless verification
./bin/godot.linuxbsd.editor.x86_64 --headless --script ~/Desktop/USF_Engine_Demo/demo.gd

# Interactive editor
./bin/godot.linuxbsd.editor.x86_64 --path ~/Desktop/USF_Engine_Demo

# Generate sample connectome data
python3 modules/usf_engine/connectome/import_flywire.py --generate-sample
```

---

## 13. Verification Results

All results from the 100-neuron sample dataset (fafb_sample.bin):

| Metric | Value | Week | Status |
|---|---|---|---|
| Vertices loaded | 100 | 1 | ✅ |
| Edges loaded | 497 | 1 | ✅ |
| Tetrahedra loaded | 140 | 2 | ✅ |
| Rich-club hubs detected | 3 | 5 | ✅ |
| Membrane potential | −65.0 mV | 3 | ✅ |
| Mean firing rate | 5.86 Hz | 3 | ✅ |
| VFE after 100 steps | 28,051 | 3 | ✅ |
| VFE after 200 steps | 42,210 | 4 | ✅ |
| Global efficiency | positive | 4 | ✅ |
| φ field (phenomenological) | 0.6 | 5 | ✅ |
| φ synchrony (Kuramoto) | 0.999 | 5 | ✅ |
| V_network (vitality) | 0.15 | 6 | ✅ |
| Senescent neurons | 0 | 6 | ✅ |
| HALT risk neurons | 0 | 6 | ✅ |
| Temporal velocity V_T | 11× | 7 | ✅ |
| Time dilation factor | 10.99× | 7 | ✅ |

---

## 14. Unit System

All internal computation uses natural units (ℏ = c = 1). The USFConstants Resource provides SI conversion:

| Constant | Value | Unit |
|---|---|---|
| ℓ_Pl | 1.616×10⁻³⁵ | m |
| t_Pl | 5.391×10⁻⁴⁴ | s |
| M_Pl | 2.176×10⁻⁸ | kg |
| κ | √(8πG) = 4.09×10⁻⁵ | kg⁻¹/²·m¹/² |
| Λ (Lee-Wick) | M_Pl/100 | J |
| α | 1.0 | (tunable) |
| Φ₀ | ρ_Pl | J/m³ |
| k_B | 1.381×10⁻²³ | J/K |

---

## 15. References

- **Unified ToE Paper**: `/home/cinni/DigitalBiology/Unified_ToE_Paper.tex` — Torres (2026), Palatini–Einstein–Cartan geometry, gauge unification, quantum completeness, and phenomenology
- **FlyWire**: Dorkenwald et al. (2024), "Neuronal wiring diagram of an adult brain" — ~139K neuron FAFB v783 connectome
- **Codex**: `https://codex.flywire.ai` — Connectome data explorer and CSV downloads
- **Godot Engine**: `https://github.com/godotengine/godot` — v4.6-stable, MIT license
- **Repository**: `https://github.com/ShrekDino/usf-engine` — private, 58 source files, ~5,000 lines
- **Built with**: [opencode](https://opencode.ai) — AI-native CLI development, 2-day implementation
