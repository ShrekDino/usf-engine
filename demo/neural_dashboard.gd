extends Node3D

@export var world: USFWorld

var blanket: DistributedBlanket
var graph: ConnectomeGraph
var multi_mesh_instance: MultiMeshInstance3D
var camera: Camera3D

var neuropil_centers: Array[Vector3] = []
var neuropil_names: Array[String] = []
var neuron_positions: PackedVector3Array
var neuron_neuropil_idx: PackedInt32Array
var neuron_indices_in_neuropil: PackedInt32Array

var neuron_count: int = 0
var neuropil_count: int = 0

var orbit_angle_x: float = 0.5
var orbit_angle_y: float = 0.0
var orbit_radius: float = 50.0
var orbit_target: Vector3 = Vector3.ZERO
var dragging: bool = false
var drag_last_pos: Vector2

var lod_distance_threshold: float = 30.0
var global_sphere_radius: float = 25.0

var is_sample_phase: bool = true


func _ready() -> void:
    if not world:
        world = _find_world()
    if not world or not world.is_initialized():
        push_error("NeuralDashboard: USFWorld not available")
        return

    var dqfr = _find_dqfr()
    if not dqfr:
        push_error("NeuralDashboard: No DQFRController found")
        return

    blanket = dqfr.get_distributed_blanket_ref()
    if not blanket or blanket.get_neuron_count() == 0:
        push_error("NeuralDashboard: DistributedBlanket unavailable or empty")
        return

    graph = blanket.graph
    if not graph or graph.get_vertex_count() == 0:
        push_error("NeuralDashboard: ConnectomeGraph unavailable or empty")
        return

    neuron_count = blanket.get_neuron_count()
    _setup_camera()
    _build_neuropil_layout()
    _setup_multimesh()
    print("[NeuralDashboard] Ready: %d neurons, %d neuropils" % [neuron_count, neuropil_count])


func _find_world() -> USFWorld:
    var p = get_parent()
    while p:
        if p is USFWorld:
            return p
        p = p.get_parent()
    return null

func _find_dqfr() -> DQFRController:
    if not world:
        return null
    for child in world.get_children():
        if child is DQFRController:
            return child
    return null


func _setup_camera() -> void:
    camera = Camera3D.new()
    camera.current = false
    camera.near = 0.1
    camera.far = 500.0
    add_child(camera)


func _build_neuropil_layout() -> void:
    neuropil_count = graph.get_neuropil_count()
    if neuropil_count == 0:
        push_error("NeuralDashboard: No neuropils found")
        return

    neuropil_centers.resize(neuropil_count)
    neuropil_names.resize(neuropil_count)

    for i in range(neuropil_count):
        neuropil_names[i] = graph.get_neuropil_by_idx(i)

    var centers = _fibonacci_sphere(neuropil_count, global_sphere_radius)
    for i in range(neuropil_count):
        neuropil_centers[i] = centers[i]

    neuron_positions.resize(neuron_count)
    neuron_neuropil_idx.resize(neuron_count)
    neuron_indices_in_neuropil.resize(neuron_count)

    for np in range(neuropil_count):
        var verts = graph.get_vertices_by_neuropil(neuropil_names[np])
        var center = neuropil_centers[np]
        var count = verts.size()
        var spread = clamp(1.0 + sqrt(float(count)) * 0.3, 1.0, 8.0)

        for j in range(count):
            var vi = verts[j]
            if vi < 0 or vi >= neuron_count:
                continue
            var offset = Vector3(
                randf_range(-spread, spread),
                randf_range(-spread, spread),
                randf_range(-spread, spread)
            )
            neuron_positions[vi] = center + offset
            neuron_neuropil_idx[vi] = np
            neuron_indices_in_neuropil[vi] = j


func _setup_multimesh() -> void:
    multi_mesh_instance = MultiMeshInstance3D.new()
    multi_mesh_instance.name = "NeuronMultiMesh"
    add_child(multi_mesh_instance)

    var sphere = SphereMesh.new()
    sphere.radius = 0.15
    sphere.height = 0.3
    sphere.rings = 2
    sphere.radial_segments = 4

    var mat = StandardMaterial3D.new()
    mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    mat.vertex_color_use_as_albedo = true
    sphere.material = mat

    var mm = MultiMesh.new()
    mm.transform_format = MultiMesh.TRANSFORM_3D
    mm.use_colors = true
    mm.instance_count = neuron_count
    mm.mesh = sphere

    for i in range(neuron_count):
        var t = Transform3D(Basis.IDENTITY, neuron_positions[i])
        mm.set_instance_transform(i, t)
        mm.set_instance_color(i, Color(0.2, 0.3, 0.8, 1.0))

    multi_mesh_instance.multimesh = mm


func _process(delta: float) -> void:
    if not blanket or neuron_count == 0:
        return

    _update_camera(delta)

    var dqfr = _find_dqfr()
    is_sample_phase = dqfr and dqfr.get_phase() == 1

    if is_sample_phase:
        _update_neuron_colors()


func _update_neuron_colors() -> void:
    var mm = multi_mesh_instance.multimesh
    if not mm:
        return

    for i in range(neuron_count):
        var rate = blanket.get_firing_rate(i)
        var vm = blanket.get_membrane_potential(i)
        var color = _firing_to_color(rate, vm)
        mm.set_instance_color(i, color)


func _firing_to_color(rate: float, _vm: float) -> Color:
    var t = clamp(rate / 120.0, 0.0, 1.0)
    var r: float
    var g: float
    var b: float

    if t < 0.25:
        var u = t / 0.25
        r = 0.1 + u * 0.0
        g = 0.2 + u * 0.4
        b = 0.8 - u * 0.2
    elif t < 0.5:
        var u = (t - 0.25) / 0.25
        r = 0.1 + u * 0.7
        g = 0.6 + u * 0.2
        b = 0.6 - u * 0.5
    elif t < 0.75:
        var u = (t - 0.5) / 0.25
        r = 0.8 + u * 0.1
        g = 0.8 - u * 0.6
        b = 0.1 - u * 0.1
    else:
        var u = (t - 0.75) / 0.25
        r = 0.9 + u * 0.1
        g = 0.2 - u * 0.15
        b = 0.0

    return Color(clamp(r, 0.0, 1.0), clamp(g, 0.0, 1.0), clamp(b, 0.0, 1.0), 1.0)


func _update_camera(delta: float) -> void:
    if not camera:
        return
    var offset = Vector3(
        orbit_radius * sin(orbit_angle_x) * cos(orbit_angle_y),
        orbit_radius * sin(orbit_angle_y),
        orbit_radius * cos(orbit_angle_x) * cos(orbit_angle_y)
    )
    camera.global_position = orbit_target + offset
    camera.look_at(orbit_target)


func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventMouseButton:
        if event.button_index == MOUSE_BUTTON_LEFT:
            dragging = event.pressed
            drag_last_pos = event.position
        if event.button_index == MOUSE_BUTTON_WHEEL_UP:
            orbit_radius *= 0.9
        if event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
            orbit_radius *= 1.1
        orbit_radius = clamp(orbit_radius, 5.0, 200.0)

    if event is InputEventMouseMotion and dragging:
        var rel = event.position - drag_last_pos
        drag_last_pos = event.position
        orbit_angle_x += rel.x * 0.005
        orbit_angle_y += rel.y * 0.005
        orbit_angle_y = clamp(orbit_angle_y, -1.4, 1.4)


func get_neuropil_center(np_name: String) -> Vector3:
    for i in range(neuropil_count):
        if neuropil_names[i] == np_name:
            return neuropil_centers[i]
    return Vector3.ZERO


func get_neuropil_names() -> Array[String]:
    return neuropil_names.duplicate()


func _fibonacci_sphere(n: int, radius: float) -> Array[Vector3]:
    var points: Array[Vector3] = []
    var phi_golden = (1.0 + sqrt(5.0)) * 0.5
    for i in range(n):
        var theta = acos(1.0 - 2.0 * (float(i) + 0.5) / float(n))
        var phi_angle = 2.0 * PI * float(i) / phi_golden
        points.append(Vector3(
            radius * sin(theta) * cos(phi_angle),
            radius * sin(theta) * sin(phi_angle),
            radius * cos(theta)
        ))
    return points
