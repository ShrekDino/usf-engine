extends Node3D

# ============================================================
# Autopoietic Sandbox — USF Engine Demo Scene
# ============================================================
# Validates O(N·k) scale-invariance, thermodynamic homeostasis,
# torsion-driven bounce, and DQFR temporal velocity modulation.
# ============================================================

# ----- Core Components -----
var world: USFWorld
var dqfr: DQFRController
var pec: PECSolver
var bounce: BounceSolver

# ----- Diagnostic State -----
var vfe_history: Array[float] = []
var eir_history: Array[float] = []
var omega_history: Array[float] = []
var scale_history: Array[float] = []
var history_max: int = 500

var diagnostic_timer: float = 0.0
var diagnostic_interval: float = 0.1

# ----- Test Configuration -----
@export var lattice_resolution: int = 4
@export var drift_duration: float = 1.0
@export var sample_duration: float = 0.1
@export var enable_stress_test: bool = false
@export var stress_burst_interval: float = 5.0

# ----- Metrics Exposed to Inspector -----
@export var current_vfe: float = 0.0
@export var current_eir: float = 0.0
@export var current_omega: float = 0.0
@export var current_vt: float = 0.0
@export var current_amin: float = 0.0
@export var current_metabolic_eta: float = 0.0
@export var current_dqfr_ratio: float = 1.0
@export var bounce_detected: bool = false
@export var node_count: int = 0
@export var edge_count: int = 0
@export var cycle_count: int = 0

# ----- Stress Test -----
var stress_timer: float = 0.0


func _ready() -> void:
    # ----- Initialize World -----
    world = USFWorld.new()
    world.lattice_n = lattice_resolution
    add_child(world)
    world.initialize_world()

    # Acquire references to sub-components
    for child in world.get_children():
        if child is DQFRController:
            dqfr = child
            dqfr.drift_duration = drift_duration
            dqfr.sample_duration = sample_duration
        elif child is PECSolver:
            pec = child
        elif child is BounceSolver:
            bounce = child

    node_count = world.complex.vertex_count()
    edge_count = world.complex.edge_count()

    # ----- Seed with a single Soliton -----
    var soliton = Soliton.new()
    soliton.constants = world.constants
    soliton.build(1.0e30, 1)
    print("[USF] Soliton initialized — compactness: %.4f, stable: %s" % [soliton.compactness(), str(soliton.topological_stability())])

    # ----- Horizon Membrane -----
    var bh = HorizonMembrane.new()
    bh.constants = world.constants
    bh.build(1.989e30, 0.0)
    print("[USF] Solar-mass BH — delay: %.6e s, freq: %.4f Hz" % [bh.echo_delay, bh.echo_frequency()])

    # ----- Stochastic burst timer -----
    stress_timer = stress_burst_interval

    print("[USF] Sandbox ready — %d nodes, %d edges, O(N·k) active" % [node_count, edge_count])
    print("[USF] DQFR: Δt=%.3f / τ=%.3f, V_T=%.4f" % [drift_duration, sample_duration, dqfr.get_temporal_velocity()])


func _process(delta: float) -> void:
    if not world.is_initialized():
        return

    # ----- Collect Diagnostics -----
    current_vfe = world.variational_free_energy
    current_omega = world.omega_coherence
    current_vt = world.temporal_velocity
    current_amin = world.bounce_a_min
    current_metabolic_eta = world.metabolic_efficiency
    current_dqfr_ratio = world.dqfr_ratio
    bounce_detected = world.bounce_detected
    cycle_count = int(dqfr.cycle_count)

    # Energy-to-Information Ratio (bits per joule)
    if current_vfe > 0.0 and current_metabolic_eta > 0.0:
        current_eir = current_metabolic_eta / (current_vfe + 1e-30)
    else:
        current_eir = 0.0

    # ----- Log History -----
    diagnostic_timer += delta
    if diagnostic_timer >= diagnostic_interval:
        diagnostic_timer = 0.0
        vfe_history.append(current_vfe)
        eir_history.append(current_eir)
        omega_history.append(current_omega)
        scale_history.append(bounce.a if bounce else 1.0)

        if vfe_history.size() > history_max:
            vfe_history.pop_front()
            eir_history.pop_front()
            omega_history.pop_front()
            scale_history.pop_front()

    # ----- Stress Test (burst flux into blanket) -----
    if enable_stress_test:
        stress_timer -= delta
        if stress_timer <= 0.0:
            stress_timer = stress_burst_interval
            _inject_flux_burst()


func _inject_flux_burst() -> void:
    # Simulate environmental burst — inject high-entropy observation
    var burst_obs: Array[float] = []
    for i in range(dqfr.inference.n_observations):
        burst_obs.append(randf())

    if dqfr.blanket:
        dqfr.blanket.unshield(burst_obs)

    # Trigger an accelerated PEC solve
    if pec:
        var conv = pec.solve_step()
        print("[STRESS] Flux burst — VFE: %.4f, PEC conv: %.6e" % [current_vfe, conv])

    # Check Forman-Ricci stability
    if dqfr.forman_ricci and dqfr.complex:
        var curvatures = dqfr.forman_ricci.compute_edge_curvatures(dqfr.complex)
        var max_curv: float = 0.0
        for c in curvatures:
            if abs(c) > max_curv:
                max_curv = abs(c)
        print("[STRESS] Max |RicF|: %.6e — manifold stable: %s" % [max_curv, str(max_curv < 1e6)])


func _input(event: InputEvent) -> void:
    # Toggle DQFR scaling with 'D' key
    if event is InputEventKey and event.pressed and event.keycode == KEY_D:
        if dqfr:
            if dqfr.drift_duration < 10.0:
                dqfr.drift_duration *= 2.0
                print("[CTRL] DQFR drift scaled to %.3f (V_T = %.4f)" % [dqfr.drift_duration, dqfr.temporal_velocity])
            else:
                dqfr.drift_duration = 1.0
                print("[CTRL] DQFR drift reset to %.3f" % dqfr.drift_duration)

    # Inject flux burst with 'F' key
    if event is InputEventKey and event.pressed and event.keycode == KEY_F:
        _inject_flux_burst()

    # Print diagnostics with 'P' key
    if event is InputEventKey and event.pressed and event.keycode == KEY_P:
        _print_diagnostics()


func _print_diagnostics() -> void:
    print("\n========== USF DIAGNOSTICS ==========")
    print(" VFE:          %.6f" % current_vfe)
    print(" EIR:          %.6f bits/J" % current_eir)
    print(" Ω_coherence:  %.6e" % current_omega)
    print(" V_T:          %.4f" % current_vt)
    print(" a_min:        %.6e (×ℓ_Pl: %.2f)" % [current_amin, current_amin / world.constants.planck_length])
    print(" η_metabolic:  %.4f" % current_metabolic_eta)
    print(" R_DQFR:       %.4f" % current_dqfr_ratio)
    print(" Bounce:       %s" % str(bounce_detected))
    print(" Cycle:        %d" % cycle_count)
    print(" Nodes:        %d" % node_count)
    print(" Edges:        %d" % edge_count)
    print("======================================\n")


# Thermodynamic stability check
func is_homeostatic() -> bool:
    var vfe_stable = false
    if vfe_history.size() >= 10:
        var recent = vfe_history.slice(vfe_history.size() - 10, vfe_history.size())
        var mean: float = 0.0
        for v in recent:
            mean += v
        mean /= recent.size()
        var variance: float = 0.0
        for v in recent:
            variance += (v - mean) * (v - mean)
        variance /= recent.size()
        vfe_stable = variance < 0.1

    return vfe_stable and world.metabolic_efficiency > 0.0 and current_omega > 0.0
