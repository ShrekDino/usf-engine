# USF Engine — ARCHITECTURE.md

## Project Structure

```
universesandbox/
├── godot-engine/                    # Modified Godot 4.6 (compiled binary + source)
│   ├── bin/godot.linuxbsd.editor.x86_64   # Compiled binary with USF module
│   ├── modules/usf_engine/                # USF Engine C++ module (compiled into binary)
│   ├── demo/
│   │   ├── bootstrap_demo.gd              # 🟢 Single entry point: generates connectome, wires everything
│   │   └── bootstrap_demo.tscn            # Scene wrapper for bootstrap
│   ├── project.godot                      # Godot project configuration
│   └── run_demo.sh                        # One-click launcher
│
├── modules/usf_engine/              # Authoritative USF Engine source
│   ├── core/                         # Foundation
│   │   ├── simplicial_complex.h/.cpp       # 4D hypercubic lattice + triangulation
│   │   ├── usf_constants.h/.cpp           # Planck units, physical constants
│   │   ├── regge_tensor.h/.cpp            # Regge calculus (discrete curvature)
│   │   ├── dynamic_coherence.h/.cpp        # O(N·k) coherence radius
│   │   └── lee_wick.h/.cpp                # Lee-Wick regulator (UV completion)
│   │
│   ├── physics/                      # Spacetime physics
│   │   ├── pec_solver.h/.cpp              # Palatini-Einstein-Cartan solver
│   │   ├── bounce_solver.h/.cpp           # Big Bounce (torsion-driven)
│   │   ├── tetrad.h/.cpp                  # Vierbein fields
│   │   ├── torsion_field.h/.cpp           # Torsion tensor
│   │   ├── soliton.h/.cpp                 # Topological solitons
│   │   ├── membrane.h/.cpp                # Horizon membrane / black hole echoes
│   │   └── forman_ricci.h/.cpp            # Forman-Ricci curvature
│   │
│   ├── thermo/                       # Thermodynamics
│   │   ├── szilard_engine.h/.cpp          # Negentropy → work conversion
│   │   ├── generative_model.h/.cpp        # Active inference / VFE minimization
│   │   └── markov_blanket.h/.cpp          # Sensory/active partition
│   │
│   ├── dqfr/                         # Temporal architecture
│   │   └── dqfr_controller.h/.cpp         # DQFR duty cycle + phase transitions
│   │
│   ├── connectome/                   # Brain connectome
│   │   ├── connectome_graph.h/.cpp        # Graph structure (vertices, edges, neuropils)
│   │   ├── distributed_blanket.h/.cpp     # Per-neuron Markov blankets (138K neurons)
│   │   ├── global_workspace.h/.cpp        # Global workspace / φ* / rich-club ignition
│   │   ├── vitality_monitor.h/.cpp        # Senescence, V_network, curiosity, HALT
│   │   └── import_flywire.py              # FlyWire→binary conversion
│   │
│   ├── demo/                         # GDScript Dashboard Suite
│   │   ├── main_dashboard.gd              # 🟢 Orchestrator — spawns all subsystems
│   │   ├── neural_dashboard.gd            # 3D MultiMeshInstance3D neuropil visualization
│   │   ├── stimulus_panel.gd              # Neuropil injection UI + intensity slider
│   │   ├── vitality_hud.gd                # Rolling V_network + VFE graphs
│   │   ├── phenomenology_panel.gd         # 🆕 Qualitative experience readout
│   │   └── autopoietic_sandbox.gd         # Original debug scene (pre-dashboard)
│   │
│   ├── register_types.cpp            # ClassDB registration (21 C++ classes)
│   ├── usf_world.h/.cpp              # Top-level USFWorld Node3D
│   ├── ARCHITECTURE.md               # This file
│   ├── EXPLAIN.md                    # Human-readable explanation
│   └── README.md                     # Project README
│
└── the-grand-synthesis/             # Rust port (independent verification)
    └── src/
        ├── main.rs                   # CLI runner
        ├── lib.rs                    # Module declarations
        ├── constants.rs              # Physical constants
        ├── mesh/                     # Simplicial complex, Regge, Lee-Wick
        ├── physics/                  # PEC, tetrad, torsion, bounce, soliton
        └── thermo/                   # Szilard, active inference, Markov blanket
```

---

## Data Flow

### Initialization (bootstrap_demo.gd)

```
bootstrap_demo.gd:_ready()
  │
  ├── 1. Create USFWorld (lattice_n=2)
  │      └── world.initialize_world()
  │           ├── SimplicialComplex.from_hypercubic_lattice()
  │           ├── ReggeTensor.compute()
  │           ├── PECSolver + BounceSolver (child nodes)
  │           └── DQFRController (child node)
  │
  ├── 2. Generate/Load synthetic connectome binary
  │      ├── ConnectomeGraph.load_binary()
  │      └── (generates 1500 neurons, 10 neuropils if not cached)
  │
  ├── 3. Initialize neural blanket
  │      ├── DistributedBlanket(graph) → initialize() (1500 neurons)
  │      ├── GlobalWorkspace(graph, blanket) → initialize() (rich-club hubs)
  │      ├── VitalityMonitor(graph, blanket, workspace) → initialize()
  │      └── Assign to DQFRController.distributed_blanket / .global_workspace / .vitality_monitor
  │
  └── 4. Spawn dashboard (main_dashboard.gd)
         ├── NeuralView3D (neural_dashboard.gd)
         │    ├── Camera3D (orbit controls)
         │    └── MultiMeshInstance3D (1500 neuron spheres)
         │
         └── CanvasLayer
              ├── StimulusPanel (left)
              ├── VitalityHUD (bottom)
              └── PhenomenologyPanel (right)
```

### Runtime Frame (every `_process(delta)`)

```
main_dashboard.gd  ──resize─→ UI layout

DQFRController._process(delta)
├── Drift phase
│   ├── Freeze all neural updates
│   ├── Ban subroutine time
│   └── When drift_timer ≥ drift_duration → transition_to_sample()
│
└── Sample phase
    ├── blanket_permeability() → chi (smooth ramp)
    ├── MarkovBlanket.unshield() → open sensory boundary
    ├── PECSolver.solve_step()
    ├── FormanRicci.damp_metric()
    ├── BounceSolver.step()
    ├── GenerativeModel.update_beliefs()
    ├── LeeWickRegulator.clamp_edge_lengths()
    └── When sample_timer ≥ sample_duration → transition_to_drift()

neural_dashboard.gd  (during Sample phase only)
└── MultiMesh.set_instance_color(i, firing_to_color(rate, vm))

vitality_hud.gd  (during Sample phase only)
├── Collect VFE + V_network samples
└── queue_redraw() → _draw() polyline graphs

phenomenology_panel.gd  (every frame)
├── _update_metrics(): read blanket/vitality/workspace/dqfr
├── _evaluate_state(): state machine (10 states)
├── _generate_description(): natural language template
├── _update_ui(): mood indicator color + gauges
└── _add_log_entry() every 3s to experience log

stimulus_panel.gd  (on user click)
├── blanket.inject_sensory(vertex_idx, intensity)
├── emit stimulus_injected signal
└── phenomenology_panel.notify_stimulus() → log entry
```

---

## C++ Class Registry (21 classes)

All registered in `register_types.cpp` and accessible from GDScript:

| Class | Purpose | Key GDScript Bindings |
|---|---|---|
| `USFConstants` | Planck units, physical constants | `planck_length`, `planck_mass`, etc. |
| `SimplicialComplex` | 4D hypercubic lattice + triangulation | `from_hypercubic_lattice()`, `vertex_count()` |
| `ReggeTensor` | Discrete curvature (deficit angles) | `compute()`, `average_curvature()` |
| `LeeWickRegulator` | UV completion / edge clamping | `clamp_edge_length()` |
| `DynamicCoherence` | O(N·k) interaction radius | `omega_coherence` |
| `FormanRicci` | Edge curvature + damping | `compute_edge_curvatures()`, `damp_metric()` |
| `PECSolver` | Palatini-Einstein-Cartan gravity | `solve_step()`, `tetrads` |
| `BounceSolver` | Big Bounce dynamics | `step()`, `a_min`, `bounce_detected` |
| `Tetrad` | Vierbein / frame field | `metric_component()` |
| `TorsionField` | Torsion tensor | `compute_from_tetrad()` |
| `Soliton` | Topological solitons | `build()`, `compactness()` |
| `HorizonMembrane` | Black hole echoes | `echo_delay`, `echo_frequency()` |
| `SzilardEngine` | Negentropy → work | `step()`, `get_efficiency()` |
| `GenerativeModel` | VFE / active inference | `update_beliefs()`, `variational_free_energy()` |
| `MarkovBlanket` | Sensory/active partition | `seal()`, `unshield()`, `sensory_states` |
| `DQFRController` | Temporal duty cycle | `get_phase()`, `drift_duration`, `temporal_velocity` |
| `ConnectomeGraph` | Brain graph dataset | `load_binary()`, `get_neuropil_count()`, `get_vertices_by_neuropil()` |
| `DistributedBlanket` | 138K-neuron thermodynamics | `initialize()`, `inject_sensory()`, `get_firing_rate()`, `get_total_vfe()` |
| `GlobalWorkspace` | Rich-club / φ* / ignition | `is_ignited()`, `get_phi_star()`, `get_phi_synchrony()` |
| `VitalityMonitor` | Senescence / V_network / curiosity | `get_v_network()`, `get_senescent_count()`, `is_exploring()` |
| `USFWorld` | Top-level scene root | `initialize_world()`, `lattice_n`, `variational_free_energy` |

---

## Binary Format (synthetic connectome)

```
Offset  Size  Field
0       4     Magic: 0x55F5
4       4     Version: 2
8       4     Vertex count (V)
12      4     Edge count (E)
16      4     Tetrahedron count (T)

20      28*V  Vertex records:
                root_id  (8 bytes, uint64)
                x, y, z  (4 bytes each, float)
                cell_type_idx, side_idx, nt_type_idx, flow_idx
                         (2 bytes each, uint16)

20+28V 16*E  Edge records:
                pre_idx, post_idx  (4 bytes each, uint32)
                synapse_count      (4 bytes, float)
                neuropil_idx, nt_type_idx  (2 bytes each, uint16)

20+28V+16E  16*T  Tetrahedron records:
                v0, v1, v2, v3  (4 bytes each, uint32)

            5 string tables (count + strings):
                cell_type_table, side_table, nt_type_table,
                flow_table, neuropil_table
```

---

## State Machine (phenomenology_panel.gd)

```
                            ┌──────────────────────────┐
                            │     UNCONSCIOUS_DRIFT    │
                            │   (DQFR phase == DRIFT)  │
                            └───────────┬──────────────┘
                                        │ phase = SAMPLE
                                        ▼
              ┌─────────────────────────────────────────┐
              │         Global ignition?                 │
              │         yes ────► GLOBAL_IGNITION       │
              │         no ───── continue                │
              └─────────────────────────────────────────┘
                                        │
              ┌─────────────────────────────────────────┐
              │         HALT risk neurons?               │
              │         yes ────► SYSTEMIC_DISTRESS      │
              │         no ───── continue                │
              └─────────────────────────────────────────┘
                                        │
              ┌─────────────────────────────────────────┐
              │         Senescence > 20?                 │
              │         yes ────► AGING_STRESS           │
              │         no ───── continue                │
              └─────────────────────────────────────────┘
                                        │
              ┌─────────────────────────────────────────┐
              │         Curiosity active?                │
              │         yes ────► EXPLORATORY           │
              │         no ───── continue                │
              └─────────────────────────────────────────┘
                                        │
              ┌─────────────────────────────────────────┐
              │         Recent stimulus?                 │
              │         yes ────► SENSORY_EXPERIENCE    │
              │         no ───── continue                │
              └─────────────────────────────────────────┘
                                        │
              ┌─────────────────────────────────────────┐
              │         VFE rising fast?                 │
              │         yes ────► SURPRISE              │
              │         no ───── continue                │
              └─────────────────────────────────────────┘
                                        │
              ┌─────────────────────────────────────────┐
              │         Vitality < 0.25?                 │
              │         yes ────► THERMODYNAMIC_FATIGUE  │
              │         no ───── continue                │
              └─────────────────────────────────────────┘
                                        │
              ┌─────────────────────────────────────────┐
              │   VFE stable + Vitality > 0.5           │
              │   + Coherence > 0.4                     │
              │   yes ────► CALM_WAKEFULNESS            │
              │   no ───── continue                     │
              └─────────────────────────────────────────┘
                                        │
                                        ▼
                               IDLE_PROCESSING
```

---

## Key Design Decisions

1. **Lattice n=2 for demo** — `lattice_n=4` generates 256 hypercubes × 1820 tetrahedra each = ~41s triangulation. `n=2` completes in 11ms and is sufficient for spacetime physics.

2. **Synthetic connectome** — The real FlyWire dataset requires API authentication and is 84MB. The bootstrap generates a 1500-neuron synthetic version automatically, cached to `user://`.

3. **DQFR-gated UI** — Colors, graphs, and experience descriptions only update during Sample phase because that's when the brain is "conscious." During Drift, the UI shows the last-known state.

4. **Neuropil sphere layout** — Neurons arranged on Fibonacci-sphere neuropil clusters rather than force-directed layout (too expensive for 5.3M synapses at startup).

5. **MultiMeshInstance3D** — Single draw call for 1500 neurons. Per-instance colors updated only during Sample phase. Unshaded material for performance.
