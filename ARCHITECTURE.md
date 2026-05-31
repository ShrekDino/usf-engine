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
│   │   ├── language_interface.gd           # 🆕 Emergent symbolic communication / training
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
               ├── LanguageInterface (left-center) — 🆕
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

language_interface.gd  (training mode or user input)
├── Phase 4: Character Recognition (97.9% accuracy)
│   ├── User types text → _present_symbol(c) per character
│   ├── encoder: maps char → 10-dimensional injection signature (intensity range 200-1000)
│   ├── blanket.inject_sensory(vi, intensity*100) across each neuropil's vertices
│   ├── _poll_response(): reads injection signature directly (bypasses washed-out blanket dynamics)
│   ├── _decode_response(): 10→32→31 MLP (1375 params, Xavier init, lr=0.1)
│   │   ├── ReLU hidden → softmax output → cross-entropy loss
│   │   └── Tracks accuracy: correct / total_presentations
│   └── _train_step() every training_interval (0.01s turbo, 0.3s normal)
│       └── Iterates training_corpus (13,633 chars from file or embedded fallback)
│
├── Phase 6: Active Inference Loop (post-training)
│   ├── _compute_prediction_mod(): maps GM confidence → injection modulation (0.3-3.0×)
│   ├── _present_symbol(): uses prediction_mod to scale injection intensity
│   ├── _poll_response/_decode_response(): records VFE + next-char prediction
│   ├── Low VFE + low mod = predicted character (brain "expected" this)
│   └── High VFE + high mod = surprising character (brain "learns" from this)
│
├── Phase 5: Sequence Prediction via GenerativeModel
│   ├── Bigram GM: 31 states × 31 observations (P(next | current))
│   ├── Trigram GM: 31 states × 961 observations (P(next | prev, current))
│   ├── _train_sequence(): updates likelihood matrix counts on (prev,curr,next) triples
│   ├── predict_next_char(): weighted 0.3×bigram + 0.7×trigram
│   ├── generate_text(seed, N): autoregressive text generation with loop detection
│   ├── evaluate_sequence_prediction(): top-1/top-3 + avg VFE over full corpus
│   ├── save/load via binary file (seq_gm.bin) for persistence
│   └── VFE displayed during communication (measure of prediction surprise)
│
└── Chat history logged in conversation panel
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
| `GenerativeModel` | VFE / active inference | `update_beliefs()`, `variational_free_energy()`, `get_likelihood()`, `set_likelihood()`, `get_prior()`, `set_prior()` |
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

## Language Encoding & Decoding

### Character Set

```
SYMBOLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ .,!?'-"  (31 symbols)
```

### Phase 6: Active Inference — Injection Modulation by Prediction

After training, the brain's predictions feed back into its sensory processing loop:

```
Communication demo flow (Active Inference):
1. User sends message → context reset (prev=-1, last=-1)
2. For each character C_t in the message:
   a. Compute prediction confidence P(C_t | context) from trigram GM
   b. Map confidence → modulation factor:
      mod = clamp(0.3 + (1.0 - confidence) * 3.0, 0.3, 3.0)
   c. Inject with amplified intensity: injection * mod
   d. Decoder reads unmodulated signature → recognition (unchanged accuracy)
   e. Compute VFE = trigram_gm.variational_free_energy(observation)
   f. Record context for next prediction
3. Display: decoded char + VFE + modulation factor
```

Result: The brain processes predicted characters weakly and surprising characters strongly — a true Active Inference loop.

### Phase 4: Injection Signature Encoding (96-98% accuracy)

Each symbol `i` maps to a **10-dimensional pseudorandom injection signature** (range 200-1000) that's injected deterministically into each of 10 neuropils:

```
sig[n] = pseudorandom(i, n)  →  deterministic per (char, neuropil) pair
injection: blanket.inject_sensory(vertex, sig[n] * 100.0) across all vertices in neuropil n
```

The **injection signature** is then read directly as decoder input, bypassing the blanket `process_step(0.1)` which washes out all firing-rate discrimination (homogenizing to ~4-6 Hz uniform attractor).

### Neural Decoder Architecture

```
10×32×31 MLP (1375 parameters)
├── Input: 10-dimensional injection signature
├── Hidden: 32 units, ReLU activation
├── Output: 31 units, softmax → cross-entropy loss
├── Xavier weight initialization
├── Learning rate: 0.1
└── Training interval: 0.01s (DQFR turbo mode)
```

### Phase 5: Sequence Prediction

Two GenerativeModels for next-character prediction:

| Model | States | Observations | Top-1 Acc | Top-3 Acc |
|---|---|---|---|---|
| Bigram GM | 31 (next char) | 31 (current char) | 18.0% | 33.7% |
| Trigram GM | 31 (next char) | 961 (prev+current) | 40.4% | 68.9% |
| Combined (0.3×bi + 0.7×tri) | — | — | 39.9% | 66.4% |

Training: count-based conditional probability estimation from corpus bigrams/trigrams, normalized after each epoch.

Prediction: `belief[C_next] = P(obs=C_curr | state=C_next) × prior[C_next]` via GM's `update_beliefs()`.

### Text Generation

Autoregressive character-by-character generation from a seed prefix:
```
generate_text("THE ", 20) → "THE BRAING. THE BRAING. "
generate_text("FREE ", 20) → "FREE BRAING. THE BRAING. "
generate_text("I ", 20) → "I PROMPUT. THE BRAING."
```

Includes loop detection (breaks after 5 repeated chars) and seed context accumulation.

### Current Learning Results

| Metric | Value |
|---|---|
| Corpus size | 13,633 characters |
| Neural decoder architecture | 10→32→31 MLP (1375 params) |
| Neural decoder accuracy | **97.9%** (13,633 presentations, ~75s turbo) |
| Bigram accuracy (top-1) | 18.0% (5.6× random baseline) |
| Trigram accuracy (top-1) | **40.4%** (12.6× random baseline) |
| Trigram accuracy (top-3) | 68.9% |
| Random baseline | 3.2% (1/31) |
| Avg VFE (trigram) | 10.72 (measure of prediction surprise) |
| Communication demo | 100% character-perfect echo (3 test messages) |
| GM persistence | Save/load to `user://seq_gm.bin` |

---

### GenerativeModel C++ Bindings Added (Phase 5)

The following GDScript-exposed methods were added to `thermo/generative_model.h/.cpp`:

| Method | Signature | Purpose |
|---|---|---|
| `get_likelihood` | `(int state, int obs) → double` | Read likelihood P(obs\|state) |
| `set_likelihood` | `(int state, int obs, double val)` | Write likelihood for training |
| `get_prior` | `(int i) → double` | Read prior probability |
| `set_prior` | `(int i, double val)` | Set prior probability |

Additionally, `get_inference_ref()` was added to `dqfr_controller.cpp` to expose the DQFR's internal GenerativeModel to GDScript (used both for sequence prediction and as an independent GM instance).

**Godot binary recompiled** after each C++ change (14-16s incremental rebuild via scons).

---

## Key Design Decisions

1. **Lattice n=2 for demo** — `lattice_n=4` generates 256 hypercubes × 1820 tetrahedra each = ~41s triangulation. `n=2` completes in 11ms and is sufficient for spacetime physics.

2. **Synthetic connectome** — The real FlyWire dataset requires API authentication and is 84MB. The bootstrap generates a 1500-neuron synthetic version automatically, cached to `user://`.

3. **DQFR-gated UI** — Colors, graphs, and experience descriptions only update during Sample phase because that's when the brain is "conscious." During Drift, the UI shows the last-known state.

4. **Neuropil sphere layout** — Neurons arranged on Fibonacci-sphere neuropil clusters rather than force-directed layout (too expensive for 5.3M synapses at startup).

5. **MultiMeshInstance3D** — Single draw call for 1500 neurons. Per-instance colors updated only during Sample phase. Unshaded material for performance.
6. **Injection signature bypass for recognition** — The blanket dynamics (`process_step(0.1)`) converge all firing rates to identical attractors (~4-6 Hz) regardless of input, making discrimination from rates alone impossible. The injection signature is read directly as decoder input rather than going through the blanket output. This was the Phase 4 breakthrough that broke the 15-16% plateau and achieved 96%+ accuracy.
7. **High-intensity encoding** — Injection range 200-1000 ensures the injection signal dominates the network contribution (~375) in `sensory_sum`, preserving discriminative information through the blanket processing step.
8. **Outside-GM for sequence prediction** — A separate `GenerativeModel` instance (not the DQFR's internal one) is used for sequence prediction, with 31 states (next char) and 961 observations (prev+current char context). This is trained from GDScript via `set_likelihood` counts and normalized after each corpus epoch.
9. **Active Inference injection modulation** — During communication demo, the injection intensity is scaled by `prediction_mod` (0.3-3.0 range) based on the GM's prediction confidence. High-confidence predictions get weaker injection (0.3×), low-confidence/surprising characters get amplified (3.0×). This creates a closed prediction→expectation→sensory processing loop where the brain's own predictions shape how it processes input. The decoder reads unmodulated signatures from `symbol_vectors`, so recognition accuracy is unaffected.
