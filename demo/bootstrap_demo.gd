extends Node3D

const SYNTHETIC_BIN_PATH := "user://synthetic_connectome.bin"
const DEMO_NEURONS := 1500
const DEMO_EDGES := 7500
const DEMO_NEUROPILS := 10

var world: USFWorld
var dqfr: DQFRController
var dashboard: Node3D


func _ready() -> void:
    print("[Bootstrap] Initializing USF Engine Demo...")

    print("[Bootstrap] Step 1/5: Setting up world...")
    _setup_world()
    if not dqfr:
        return

    print("[Bootstrap] Step 2/5: Loading/generating connectome...")
    var connectome = _load_or_generate_connectome()
    if not connectome:
        return

    print("[Bootstrap] Step 3/5: Setting up neural blanket...")
    _setup_blanket(connectome)

    print("[Bootstrap] Step 4/5: Spawning dashboard...")
    _spawn_dashboard()

    print("[Bootstrap] Step 5/5: Setting up scene...")
    _setup_scene()

    print("[Bootstrap] Demo ready! Drag to orbit, scroll to zoom. Stimulus panel on left.")


func _setup_world() -> void:
    world = USFWorld.new()
    world.lattice_n = 2
    add_child(world)
    print("[Bootstrap]   Calling world.initialize_world()...")
    world.initialize_world()
    print("[Bootstrap]   World initialized, finding DQFR...")

    for child in world.get_children():
        if child is DQFRController:
            dqfr = child
            break

    if not dqfr:
        push_error("[Bootstrap] DQFRController not found after world init")
        return

    print("[Bootstrap] World initialized (lattice_n=%d)" % [world.lattice_n])


func _load_or_generate_connectome():
    var graph := ConnectomeGraph.new()

    if FileAccess.file_exists(SYNTHETIC_BIN_PATH):
        print("[Bootstrap] Loading cached synthetic connectome...")
        if graph.load_binary(SYNTHETIC_BIN_PATH):
            print("[Bootstrap] Cached connectome loaded: %d neurons" % graph.get_vertex_count())
            return graph
        else:
            print("[Bootstrap] Cache invalid, regenerating...")

    print("[Bootstrap] Generating synthetic connectome (%d neurons, %d edges, %d neuropils)..." % [DEMO_NEURONS, DEMO_EDGES, DEMO_NEUROPILS])
    _write_synthetic_binary(SYNTHETIC_BIN_PATH)
    if not graph.load_binary(SYNTHETIC_BIN_PATH):
        push_error("[Bootstrap] Failed to load generated connectome")
        return null

    print("[Bootstrap] Synthetic connectome ready: %d neurons, %d neuropils" % [
        graph.get_vertex_count(), graph.get_neuropil_count()])
    return graph


func _write_synthetic_binary(path: String) -> void:
    var f := FileAccess.open(path, FileAccess.WRITE)
    if not f:
        push_error("[Bootstrap] Cannot write " + path)
        return

    var neuropil_names := ["ME_L", "ME_R", "LO_L", "LO_R", "LOP_L", "LOP_R",
                           "AL_L", "AL_R", "MB_PED_L", "MB_PED_R"]
    var cell_types := ["UnpolarizedNeuron", "T1", "Mi1", "Tm1", "Tm5",
                       "L1", "L2", "L3", "L4", "L5"]
    var sides := ["L", "R"]
    var nt_types := ["ACh", "GABA", "Glu", "DA", "5HT"]
    var flows := ["output", "input", "mixed"]

    var nv := DEMO_NEURONS
    var ne := DEMO_EDGES
    var npc := DEMO_NEUROPILS

    f.store_32(0x55F5)
    f.store_32(2)
    f.store_32(nv)
    f.store_32(ne)
    f.store_32(0)

    var seed_val: int = 42
    var rng := RandomNumberGenerator.new()
    rng.seed = seed_val

    # --- Structured connectome: evenly distribute neurons across neuropils ---
    var v_np: Array[int] = []
    var v_ct: Array[int] = []
    var v_side: Array[int] = []
    v_np.resize(nv)
    v_ct.resize(nv)
    v_side.resize(nv)

    var np_neurons: Array = []  # list of lists: which neurons belong to each neuropil
    for np in range(npc):
        np_neurons.append([])

    for i in range(nv):
        var np_i: int = i % npc  # round-robin for even distribution
        v_np[i] = np_i
        v_ct[i] = rng.randi_range(0, cell_types.size() - 1)
        v_side[i] = rng.randi_range(0, sides.size() - 1)
        np_neurons[np_i].append(i)

        var x := rng.randf_range(-50.0, 50.0)
        var y := rng.randf_range(-50.0, 50.0)
        var z := rng.randf_range(-50.0, 50.0)
        f.store_64(72000000000000000 + i)
        f.store_float(x)
        f.store_float(y)
        f.store_float(z)
        f.store_16(v_ct[i])
        f.store_16(v_side[i])
        f.store_16(rng.randi_range(0, nt_types.size() - 1))
        f.store_16(rng.randi_range(0, flows.size() - 1))

    # --- Structured edge generation ---
    # Partner neuropil pairs (information highways)
    var partners := {
        0: 2, 2: 0,  # ME_L <-> LO_L
        1: 3, 3: 1,  # ME_R <-> LO_R
        4: 5, 5: 4,  # LOP_L <-> LOP_R
        6: 8, 8: 6,  # AL_L <-> MB_PED_L
        7: 9, 9: 7,  # AL_R <-> MB_PED_R
    }

    var edge_set := {}  # track existing edges to avoid duplicates
    var edges_written := 0

    var write_edge = func(pre: int, post: int, weight: float, nt_idx: int):
        var key = (int(pre) << 16) | int(post)
        if edge_set.has(key) or pre == post:
            return false
        edge_set[key] = true
        f.store_32(pre)
        f.store_32(post)
        f.store_float(weight)
        f.store_16(v_np[pre])
        f.store_16(nt_idx)
        edges_written += 1
        return true

    # Phase 1: Within-neuropil connections (strong, ~50% of edges)
    var within_budget = int(ne * 0.50)
    for np in range(npc):
        var neurons = np_neurons[np]
        var n_n = neurons.size()
        if n_n < 2: continue
        var per_neuron = maxi(1, within_budget / (npc * n_n))
        for ni in neurons:
            var targets := []
            for attempt in 20:
                var tj = rng.randi_range(0, n_n - 1)
                var tgt = neurons[tj]
                if tgt != ni:
                    targets.append(tgt)
                if targets.size() >= per_neuron:
                    break
            for tgt in targets:
                var w = rng.randf_range(15.0, 25.0)
                var nt = rng.randi_range(0, nt_types.size() - 1)
                if not write_edge.call(ni, tgt, w, nt):
                    continue
                if edges_written >= ne:
                    break
            if edges_written >= ne:
                break
        if edges_written >= ne:
            break

    # Phase 2: Bridge connections between partner neuropils (medium, ~35% of edges)
    var bridge_budget = int(ne * 0.35)
    for part_pair in partners.keys():
        var np_a = part_pair
        var np_b = partners[part_pair]
        var neurons_a = np_neurons[np_a]
        var neurons_b = np_neurons[np_b]
        var per_pair = maxi(1, bridge_budget / (partners.size() * 2))
        for ni in neurons_a:
            var targets := []
            for attempt in 10:
                var tj = rng.randi_range(0, neurons_b.size() - 1)
                var tgt = neurons_b[tj]
                targets.append(tgt)
                if targets.size() >= per_pair:
                    break
            for tgt in targets:
                var w = rng.randf_range(8.0, 15.0)
                var nt = rng.randi_range(0, nt_types.size() - 1)
                if not write_edge.call(ni, tgt, w, nt):
                    continue
                if edges_written >= ne:
                    break
            if edges_written >= ne:
                break
        if edges_written >= ne:
            break

    # Phase 3: Random fill for remaining edges (weak, ~15% of edges)
    while edges_written < ne:
        var pre := rng.randi_range(0, nv - 1)
        var post := rng.randi_range(0, nv - 1)
        var w = rng.randf_range(1.0, 8.0)
        var nt = rng.randi_range(0, nt_types.size() - 1)
        write_edge.call(pre, post, w, nt)

    var write_table = func(tbl: Array):
        f.store_32(tbl.size())
        for s in tbl:
            var b = s.to_utf8_buffer()
            f.store_32(b.size())
            f.store_buffer(b)

    write_table.call(cell_types)
    write_table.call(sides)
    write_table.call(nt_types)
    write_table.call(flows)
    write_table.call(neuropil_names)

    f.close()


func _setup_blanket(graph: ConnectomeGraph) -> void:
    var blanket := DistributedBlanket.new()
    blanket.graph = graph
    blanket.initialize()

    var ws := GlobalWorkspace.new()
    ws.set_graph(graph)
    ws.set_blanket(blanket)
    ws.initialize()

    var vm := VitalityMonitor.new()
    vm.set_graph(graph)
    vm.set_blanket(blanket)
    vm.set_workspace(ws)
    vm.initialize()

    dqfr.distributed_blanket = blanket
    dqfr.global_workspace = ws
    dqfr.vitality_monitor = vm

    print("[Bootstrap] Blanket initialized: %d neurons, %d rich-club hubs" % [
        blanket.get_neuron_count(), ws.get_rich_club_count()])


func _spawn_dashboard() -> void:
    dashboard = Node3D.new()
    dashboard.name = "MainDashboard"
    var nd_script = load("res://modules/usf_engine/demo/main_dashboard.gd")
    if not nd_script:
        nd_script = load("res://demo/main_dashboard.gd")
    if nd_script:
        dashboard.set_script(nd_script)
        dashboard.set("world", world)
    add_child(dashboard)


func _setup_scene() -> void:
    var light_a := DirectionalLight3D.new()
    light_a.name = "Sun"
    light_a.transform.basis = Basis().rotated(Vector3.RIGHT, -0.6).rotated(Vector3.UP, 0.3)
    add_child(light_a)

    var light_b := DirectionalLight3D.new()
    light_b.name = "Fill"
    light_b.light_energy = 0.3
    light_b.transform.basis = Basis().rotated(Vector3.RIGHT, 0.4).rotated(Vector3.UP, -0.5)
    add_child(light_b)

    var ambient := WorldEnvironment.new()
    ambient.environment = Environment.new()
    ambient.environment.ambient_light_color = Color(0.05, 0.05, 0.1)
    ambient.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    ambient.environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
    add_child(ambient)

    print("[Bootstrap] Scene lighting ready")

    _start_language_training()


func _start_language_training() -> void:
    await get_tree().create_timer(1.0).timeout
    if dqfr:
        dqfr.drift_duration = 0.01
        dqfr.sample_duration = 0.5
        print("[Bootstrap] Brain woken for language learning")
    
    await get_tree().create_timer(0.5).timeout
    if dqfr:
        dqfr.drift_duration = 0.001
        dqfr.sample_duration = 0.01
        print("[Bootstrap] DQFR turbo enabled: VT=%.0f" % dqfr.get_temporal_velocity())
    for child in get_children():
        if child.has_method("get_node"):
            var li = child.find_child("LanguageInterface", true, false)
            if li and li.has_method("_toggle_training"):
                li._toggle_training()
                print("[Bootstrap] Language training started")
                break
