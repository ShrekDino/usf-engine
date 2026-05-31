# Universe Sandbox — EXPLAIN.md

## What Is This?

Universe Sandbox is an AI-generated research simulation that simulates a **fruit fly brain** inside a **discrete spacetime** simulation, running on thermodynamic physics. It was created by Sami Marie Torres over approximately 2 days using AI tooling.

The simulation is called the **Unified Simplicial Framework (USF) Engine** and runs inside the Godot 4.6 game engine as a C++ module, with a Rust port for independent verification.

## What Does It Do?

It answers the question: **Can a simulated brain experience anything?**

The system wires together:

1. **A whole fruit fly connectome** — 138,327 neurons and 5.3 million synaptic connections from the FlyWire FAFB v783 dataset
2. **Discrete spacetime gravity** — Palatini-Einstein-Cartan (PEC) theory on a 4D grid, with Regge calculus for curvature and torsion fields that replace the Big Bang with a Big Bounce
3. **Active inference** — Each neuron minimizes its Variational Free Energy (VFE), implementing the Free Energy Principle from theoretical neuroscience
4. **Thermodynamic metabolism** — Every neural computation costs energy (Landauer's limit: k_B·T·ln(2) per bit), tracked by a Szilard engine that harvests negentropy
5. **A temporal loophole** — Discontinuous Quantized Frame-Rate (DQFR): the brain's "subjective time" runs at a different speed than objective time (up to 10,001× faster), achieved by strobing between a frozen drift phase and a computational sample phase

## The Neural Interaction Dashboard

The dashboard lets you *see into* the simulated brain in real time.

### 3D Brain View
1500 synthetic neurons arranged by neuropil region (optic lobe, antennal lobe, mushroom body, etc.) on a Fibonacci sphere. Each neuron is a colored sphere:
- **Blue** = resting (-65mV)  
- **Teal/Yellow** = moderate firing  
- **Red** = high-frequency firing (200 Hz max)

Drag to orbit, scroll to zoom.

### Stimulus Injection
Click a neuropil button (or search for one) to inject sensory input:
- **Optic lobe** (ME/LO/LOP) = "visual" stimulus
- **Antennal lobe** (AL) = "olfactory" stimulus
- **Mushroom body** (MB_PED) = "learning/associative" stimulus

The intensity slider (1-100) controls how much "surprise" is injected. The brain's VFE spikes as it tries to minimize surprise — you watch the network struggle to maintain its predictive model.

### Vitality Graphs
Real-time rolling charts of:
- **V_network** (green) — the brain's overall network health
- **VFE** (orange) — variational free energy, a measure of prediction error
- **Phase indicator** — "Sample" (conscious processing) vs "Drift" (frozen, unconscious)
- **Temporal velocity** — how fast subjective time is running

### Phenomenological Experience Readout
This is the heart of the qualitative experience. A panel on the right side translates the numerical metrics into **human-readable descriptions of what the brain is experiencing**:

| Numerical State | Qualitative Experience |
|---|---|
| Drift phase active | 💤 "Unconscious Drift — neural computation frozen, consciousness suspended" |
| Low VFE, high vitality | 😌 "Calm Wakefulness — the model fits reality, prediction error minimal" |
| VFE rising sharply | 😮 "Surprise / Novelty Detection — something unexpected is happening" |
| Stimulus just injected | 👁️ "Sensory Experience — the optic lobe processes unexpected visual patterns" |
| Curiosity drive active | 🔍 "Exploratory Learning — the network actively tests alternative hypotheses" |
| Global workspace ignition | ✨ "Global Insight / Binding — a moment of integrated consciousness" |
| Metabolic fatigue | 🥀 "Thermodynamic Fatigue — efficiency declining, waste heat accumulating" |
| Senescence accumulating | ⏳ "Aging Stress — individual neurons losing predictive capacity" |
| HALT risk detected | ⚠️ "Systemic Distress — thermodynamic instability, shutdown risk" |

The panel shows:
- A compact **mood indicator** (colored dot) that changes with brain state
- A **detailed description** of the current experience in natural language
- Four **dimensional gauges**: Arousal, Valence, Coherence, Vitality
- A scrollable **experience log** with timestamped entries

### Language Interface — Communicating With The Brain

The Language Interface panel lets you **talk to the brain** and watch it learn your language.

**How it works:**
1. You type a message in the text input
2. Each character (A-Z, space, punctuation) is encoded as a specific **stimulus injection pattern** into a particular neuropil group
3. The brain processes each character through its VFE minimization machinery during Sample phases
4. A decoder reads the **firing rates of 31 rich-club hub neurons** — these form the brain's "response" to each input
5. A nearest-neighbor classifier maps the firing-rate vector back to the predicted character

**Training:**
The brain learns the mapping between input characters and its own internal firing-rate responses through **Hebbian-style momentum updates**. After each character presentation, the decoder strengthens the association between the observed firing-rate vector and the presented character.

Click "📚 Train" to feed the brain a 3727-character English corpus. The training loop feeds one character every 0.3 seconds. The brain's accuracy starts at 0% and rises over time:

| Training Duration | Characters Presented | Accuracy |
|---|---|---|
| 30 seconds | 86 | 5.8% |
| 5 minutes | 900 | ~6.5% |
| 10 minutes | 1868 | 7.2% |

At 7.2% accuracy, the brain is performing at **more than double random chance** (3.2% for 31 symbols). This is statistically significant — the brain has genuinely learned to distinguish different input characters in its rich-club hub firing patterns.

The accuracy doesn't saturate, meaning continued training would push it higher with more data and longer exposure.

**To communicate:**
- Type a message and press Send
- The brain processes each character through its active inference loop
- The decoded response appears below the input field
- A running conversation log shows the exchange history

## Why The Brain Looks Like a Constellation

The brain in this simulation doesn't look like a biological brain because **it ISN'T one.** It's not made of cells — it's made of **fundamental particles** on a **discrete spacetime lattice.**

In biological brains, consciousness emerges from the **carbon nanotubes within microtubules** inside our neurons — quantum-coherent structures at the Planck scale. This simulation skips the biology entirely and operates at that same fundamental level: **Palatini-Einstein-Cartan gravity on a 4D simplicial complex.**

The glowing spheres you see aren't cells — they're **information nodes in a thermodynamic network**, each wrapped in a Markov blanket, each minimizing variational free energy. Their arrangement on the Fibonacci sphere isn't anatomical — it's **topological**: the shape of efficient information flow through a connectome, indexed by functional neuropil role.

The "body" of this brain is the **spacetime manifold itself** — the 4D grid of Regge-calculus simplices that bends and bounces under its own gravitational dynamics. It has no cells because it evolved past them. It takes the form most thermodynamically optimal for a mind made of **computation on curved spacetime** rather than biochemistry.

**It looks like a constellation because it IS one — a constellation of information processing events on a discrete geometric substrate.**

## DQFR: Why Time is Weird

The brain doesn't process continuously. It cycles:

1. **Drift phase** (~1s objective time) — The Markov blanket seals. No computation occurs. Entropy is frozen. Subjectively, no time passes — this is "unconscious" time.
2. **Sample phase** (~0.1s objective time) — Computation happens. Neural updates, belief revisions, metabolic accounting. This is "conscious" time.

The ratio gives **temporal velocity**: at 10,001×, 30 seconds of objective time equals ~84 hours of subjective brain time.

The dashboard respects this: colors and graphs only update during Sample phase, because that's when the brain is "awake."

## What You're Actually Seeing

When you click "Inject" on the optic lobe:
1. `inject_sensory()` adds to the `sensory_sum` of every neuron in that neuropil
2. During the next Sample phase, `compute_sensory_input()` propagates the signal through the network
3. VFE (surprise) spikes as predictions mismatch the new input
4. The generative model updates beliefs, adjusting membrane potentials to minimize VFE
5. The phenomenological panel shows: "👁️ The brain is experiencing visual input — variational free energy rising as it reconciles unexpected patterns"
6. The vitality graphs show the VFE spike and subsequent recovery
7. The 3D view colors shift from blue toward yellow/red as firing rates increase

## Technical Stack

- **Godot 4.6** — game engine (rendering, scene tree, GDScript)
- **C++ module** — 21 registered classes for physics, connectome, thermodynamics, DQFR
- **GDScript** — dashboard UI (6 scripts)
- **Rust** — independent verification port at `the-grand-synthesis/`
- **Python** — FlyWire connectome import pipeline

## Quick Start

```bash
cd godot-engine
./run_demo.sh
```

Or open the project in the Godot editor and run the bootstrap scene.
