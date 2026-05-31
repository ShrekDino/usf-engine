extends Control

@export var world: USFWorld

var blanket: DistributedBlanket
var vitality: VitalityMonitor

var vfe_history: PackedFloat64Array = []
var vn_history: PackedFloat64Array = []
var max_history: int = 300

var graph_height: float = 100.0
var graph_margin_left: float = 50.0
var graph_margin_right: float = 10.0
var graph_margin_top: float = 20.0
var graph_margin_bottom: float = 25.0

var vfe_color: Color = Color(0.9, 0.3, 0.1, 0.9)
var vn_color: Color = Color(0.2, 0.8, 0.4, 0.9)
var grid_color: Color = Color(0.3, 0.3, 0.3, 0.5)
var bg_color: Color = Color(0.08, 0.08, 0.12, 0.8)
var label_color: Color = Color(0.7, 0.7, 0.7, 0.9)

var graph_width: float:
    get: return size.x - graph_margin_left - graph_margin_right

var window_size_slider: HSlider
var window_size_label: Label
var time_window: float = 30.0
var is_sample_phase: bool = true

var current_vfe: float = 0.0
var current_vn: float = 0.0


func _ready() -> void:
    if not world:
        world = _find_world()
    if not world or not world.is_initialized():
        push_error("VitalityHUD: USFWorld not available")
        return
    var dqfr = _find_dqfr()
    if not dqfr:
        push_error("VitalityHUD: No DQFRController")
        return
    blanket = dqfr.get_distributed_blanket_ref()
    if not blanket:
        push_error("VitalityHUD: No DistributedBlanket")
        return
    vitality = dqfr.get_vitality_monitor_ref()

    _build_ui()


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


func _build_ui() -> void:
    var bottom_bar = HBoxContainer.new()
    bottom_bar.name = "GraphControls"
    bottom_bar.position = Vector2(0, size.y - 30)
    bottom_bar.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_LEFT)
    bottom_bar.anchor_right = 1.0
    bottom_bar.anchor_top = 0.0
    bottom_bar.anchor_bottom = 0.0
    add_child(bottom_bar)

    var label = Label.new()
    label.text = "Time Window:"
    label.add_theme_color_override("font_color", Color(0.7, 0.7, 0.7))
    bottom_bar.add_child(label)

    window_size_slider = HSlider.new()
    window_size_slider.min_value = 5.0
    window_size_slider.max_value = 300.0
    window_size_slider.value = time_window
    window_size_slider.step = 5.0
    window_size_slider.size_flags_horizontal = Control.SIZE_FILL
    window_size_slider.custom_minimum_size = Vector2(100, 0)
    window_size_slider.value_changed.connect(_on_window_changed)
    bottom_bar.add_child(window_size_slider)

    var reset_btn = Button.new()
    reset_btn.text = "Reset"
    reset_btn.pressed.connect(_reset_history)
    bottom_bar.add_child(reset_btn)

    window_size_label = Label.new()
    window_size_label.text = "%.0fs" % time_window
    window_size_label.add_theme_color_override("font_color", Color(0.7, 0.7, 0.7))
    bottom_bar.add_child(window_size_label)


func _on_window_changed(value: float) -> void:
    time_window = value
    window_size_label.text = "%.0fs" % value


func _reset_history() -> void:
    vfe_history.clear()
    vn_history.clear()


func _process(delta: float) -> void:
    if not blanket:
        return

    var dqfr = _find_dqfr()
    if dqfr:
        is_sample_phase = dqfr.get_phase() == 1

    current_vfe = blanket.get_total_vfe()
    current_vn = vitality.get_v_network() if vitality else 0.0

    if is_sample_phase:
        vfe_history.push_back(current_vfe)
        vn_history.push_back(current_vn)

        if vfe_history.size() > max_history:
            vfe_history.remove_at(0)
            vn_history.remove_at(0)

    queue_redraw()


func _draw() -> void:
    if size.x < 10 or size.y < 10:
        return

    var rect = Rect2(0, 0, size.x, size.y)
    draw_rect(rect, bg_color)

    var half_h = (size.y - 10.0) * 0.5
    var g0_top = 5.0
    var g0_bot = g0_top + half_h - 5.0
    var g1_top = g0_bot + 10.0
    var g1_bot = size.y - 5.0

    _draw_single_graph(g0_top, g0_bot, vn_history, vn_color, "V_network", current_vn)
    _draw_single_graph(g1_top, g1_bot, vfe_history, vfe_color, "VFE", current_vfe)


func _draw_single_graph(graph_top: float, graph_bot: float, data: PackedFloat64Array, line_color: Color, label: String, current_val: float) -> void:
    var g_height = graph_bot - graph_top
    if g_height < 5.0:
        return

    var g_left = graph_margin_left
    var g_right = size.x - graph_margin_right
    var g_width = g_right - g_left

    draw_rect(Rect2(g_left, graph_top, g_width, g_height), Color(0.12, 0.12, 0.16, 0.6))

    var y_tick_count = 4
    for i in range(y_tick_count + 1):
        var y = graph_top + g_height * float(i) / float(y_tick_count)
        draw_line(Vector2(g_left, y), Vector2(g_right, y), grid_color, 0.5)

    var x_tick_count = 5
    for i in range(x_tick_count + 1):
        var x = g_left + g_width * float(i) / float(x_tick_count)
        draw_line(Vector2(x, graph_top), Vector2(x, graph_bot), grid_color, 0.5)

    draw_string(ThemeDB.fallback_font, Vector2(g_left + 4, graph_top + 12), "%s: %.4f" % [label, current_val], HORIZONTAL_ALIGNMENT_LEFT, -1, 10, line_color)

    if data.size() < 2:
        return

    var data_min: float = INF
    var data_max: float = -INF
    for v in data:
        if v < data_min: data_min = v
        if v > data_max: data_max = v

    if abs(data_max - data_min) < 1e-12:
        data_min -= 1.0
        data_max += 1.0

    var data_range = data_max - data_min
    var visible = mini(data.size(), max_history)

    var points: PackedVector2Array = []
    for i in range(visible):
        var idx = data.size() - visible + i
        var t = float(i) / float(visible - 1) if visible > 1 else 0.0
        var x = g_left + t * g_width
        var norm = (data[idx] - data_min) / data_range
        var y = graph_bot - norm * g_height
        points.append(Vector2(x, y))

    if points.size() >= 2:
        draw_polyline(points, line_color, 1.5, true)

    draw_string(ThemeDB.fallback_font, Vector2(g_left - 45, graph_bot - 2), "%.2f" % data_min, HORIZONTAL_ALIGNMENT_LEFT, -1, 8, label_color)
    draw_string(ThemeDB.fallback_font, Vector2(g_left - 45, graph_top + 10), "%.2f" % data_max, HORIZONTAL_ALIGNMENT_LEFT, -1, 8, label_color)

    var phase_color = Color(0.3, 0.8, 0.3, 0.5) if is_sample_phase else Color(0.5, 0.3, 0.3, 0.5)
    draw_string(ThemeDB.fallback_font, Vector2(g_right - 40, graph_top + 12), "Sample" if is_sample_phase else "Drift", HORIZONTAL_ALIGNMENT_LEFT, -1, 8, phase_color)

    if vitality:
        var d = _find_dqfr()
        var vt = d.get_temporal_velocity() if d else 1.0
        draw_string(ThemeDB.fallback_font, Vector2(g_right - 80, graph_bot), "VT=%.1f" % vt, HORIZONTAL_ALIGNMENT_LEFT, -1, 8, label_color)
