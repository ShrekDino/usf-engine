extends Node3D

@export var world: USFWorld

var neural_view: Node3D
var stimulus_ui: Control
var vitality_ui: Control
var phenomenology_ui: Control
var language_ui: Control
var canvas: CanvasLayer

var dqfr: DQFRController
var current_phase: int = 0

var initialized: bool = false


func _ready() -> void:
    if not world:
        world = _find_world()
    if not world or not world.is_initialized():
        push_error("MainDashboard: USFWorld not available")
        return

    dqfr = _find_dqfr()
    if not dqfr:
        push_error("MainDashboard: No DQFRController in world")
        return

    neural_view = Node3D.new()
    neural_view.name = "NeuralView3D"
    var nd_script = load("res://modules/usf_engine/demo/neural_dashboard.gd")
    if nd_script:
        neural_view.set_script(nd_script)
        neural_view.world = world
    add_child(neural_view)

    if neural_view.get("camera"):
        var cam = neural_view.camera as Camera3D
        if cam:
            cam.current = true

    canvas = CanvasLayer.new()
    canvas.layer = 128
    canvas.follow_viewport_enabled = true
    add_child(canvas)

    stimulus_ui = Control.new()
    stimulus_ui.name = "StimulusPanel"
    var sp_script = load("res://modules/usf_engine/demo/stimulus_panel.gd")
    if sp_script:
        stimulus_ui.set_script(sp_script)
        stimulus_ui.world = world
    stimulus_ui.position = Vector2(10, 10)
    stimulus_ui.custom_minimum_size = Vector2(220, 0)
    canvas.add_child(stimulus_ui)

    vitality_ui = Control.new()
    vitality_ui.name = "VitalityHUD"
    var vh_script = load("res://modules/usf_engine/demo/vitality_hud.gd")
    if vh_script:
        vitality_ui.set_script(vh_script)
        vitality_ui.world = world
    canvas.add_child(vitality_ui)

    phenomenology_ui = Control.new()
    phenomenology_ui.name = "PhenomenologyPanel"
    var pp_script = load("res://modules/usf_engine/demo/phenomenology_panel.gd")
    if pp_script:
        phenomenology_ui.set_script(pp_script)
        phenomenology_ui.set("world", world)
    canvas.add_child(phenomenology_ui)

    language_ui = Control.new()
    language_ui.name = "LanguageInterface"
    var li_script = load("res://modules/usf_engine/demo/language_interface.gd")
    if li_script:
        language_ui.set_script(li_script)
        language_ui.set("world", world)
    language_ui.position = Vector2(240, 10)
    language_ui.custom_minimum_size = Vector2(300, 0)
    canvas.add_child(language_ui)

    if stimulus_ui.has_signal("stimulus_injected") and phenomenology_ui.has_method("notify_stimulus"):
        stimulus_ui.stimulus_injected.connect(
            func(np, intensity):
                phenomenology_ui.notify_stimulus(np, intensity)
        )

    initialized = true
    print("[MainDashboard] Neural Interaction Dashboard ready")
    print("[MainDashboard] Phases: Drift=%.3fs / Sample=%.3fs, VT=%.1f" % [
        dqfr.drift_duration, dqfr.sample_duration, dqfr.get_temporal_velocity()])


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


func _process(_delta: float) -> void:
    if not initialized:
        return

    current_phase = dqfr.get_phase() if dqfr else 0
    _resize_ui()


func _resize_ui() -> void:
    var vs = get_viewport().get_visible_rect().size if get_viewport() else Vector2(1024, 768)
    if language_ui:
        language_ui.size = Vector2(vs.x * 0.22, vs.y * 0.5)
        language_ui.position = Vector2(10 + 240, 10)
    if vitality_ui:
        vitality_ui.position = Vector2(10, vs.y * 0.55)
        vitality_ui.custom_minimum_size = Vector2(vs.x - 580, vs.y * 0.42)
        vitality_ui.size = Vector2(vs.x - 580, vs.y * 0.42)
    if phenomenology_ui:
        phenomenology_ui.size = Vector2(320, vs.y)
        phenomenology_ui.position = Vector2(vs.x - 320, 0)


func get_current_phase() -> int:
    return current_phase


func is_sample_phase() -> bool:
    return current_phase == 1
