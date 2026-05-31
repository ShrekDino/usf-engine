extends Control

@export var world: USFWorld

var blanket: DistributedBlanket
var graph: ConnectomeGraph

var neuropil_names: Array[String] = []
var neuropil_vertex_counts: Array[int] = []
var top_neuropil_count: int = 16

var intensity_slider: HSlider
var intensity_label: Label
var status_label: Label
var search_edit: LineEdit
var search_dropdown: OptionButton
var button_container: VBoxContainer

var neuropil_buttons: Array[Button] = []

signal stimulus_injected(neuropil_name: String, intensity: float)

var last_injection_neuropil: String = ""
var last_injection_intensity: float = 0.0

var panel_width: float = 220.0


func _ready() -> void:
    if not world:
        world = _find_world()
    if not world or not world.is_initialized():
        push_error("StimulusPanel: USFWorld not available")
        return
    var dqfr = _find_dqfr()
    if not dqfr:
        push_error("StimulusPanel: No DQFRController")
        return
    blanket = dqfr.get_distributed_blanket_ref()
    if not blanket:
        push_error("StimulusPanel: No DistributedBlanket")
        return
    graph = blanket.graph
    if not graph:
        push_error("StimulusPanel: No ConnectomeGraph")
        return

    _gather_neuropils()
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


func _gather_neuropils() -> void:
    var np_count = graph.get_neuropil_count()
    for i in range(np_count):
        var name = graph.get_neuropil_by_idx(i)
        var verts = graph.get_vertices_by_neuropil(name)
        neuropil_names.append(name)
        neuropil_vertex_counts.append(verts.size())

    var combined: Array[Dictionary] = []
    for i in range(np_count):
        combined.append({"name": neuropil_names[i], "count": neuropil_vertex_counts[i]})
    combined.sort_custom(func(a, b): return a.count > b.count)

    neuropil_names.clear()
    neuropil_vertex_counts.clear()
    for entry in combined:
        neuropil_names.append(entry.name)
        neuropil_vertex_counts.append(entry.count)


func _build_ui() -> void:
    var panel = VBoxContainer.new()
    panel.name = "StimulusPanel"
    panel.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
    panel.size_flags_vertical = Control.SIZE_SHRINK_END
    panel.anchor_right = 0.0
    panel.anchor_bottom = 0.0
    add_child(panel)

    var title = Label.new()
    title.text = "STIMULUS INJECTION"
    title.add_theme_font_size_override("font_size", 14)
    title.add_theme_color_override("font_color", Color(0.9, 0.9, 0.9))
    panel.add_child(title)

    panel.add_child(HSeparator.new())

    var intensity_row = HBoxContainer.new()
    var intensity_title = Label.new()
    intensity_title.text = "Intensity:"
    intensity_title.add_theme_color_override("font_color", Color(0.8, 0.8, 0.8))
    intensity_row.add_child(intensity_title)

    intensity_label = Label.new()
    intensity_label.text = "25"
    intensity_label.add_theme_color_override("font_color", Color(1.0, 1.0, 0.6))
    intensity_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
    intensity_row.add_child(intensity_label)
    intensity_row.add_theme_constant_override("separation", 4)
    panel.add_child(intensity_row)

    intensity_slider = HSlider.new()
    intensity_slider.min_value = 1.0
    intensity_slider.max_value = 100.0
    intensity_slider.value = 25.0
    intensity_slider.step = 1.0
    intensity_slider.size_flags_horizontal = Control.SIZE_FILL
    intensity_slider.value_changed.connect(_on_intensity_changed)
    panel.add_child(intensity_slider)

    panel.add_child(HSeparator.new())

    var search_label = Label.new()
    search_label.text = "Search neuropil:"
    search_label.add_theme_color_override("font_color", Color(0.8, 0.8, 0.8))
    panel.add_child(search_label)

    search_edit = LineEdit.new()
    search_edit.placeholder_text = "Type to filter..."
    search_edit.text_changed.connect(_on_search_changed)
    panel.add_child(search_edit)

    var dropdown_row = HBoxContainer.new()
    search_dropdown = OptionButton.new()
    search_dropdown.size_flags_horizontal = Control.SIZE_FILL
    search_dropdown.add_item("-- Select neuropil --")
    for name in neuropil_names:
        search_dropdown.add_item(name)
    search_dropdown.item_selected.connect(_on_dropdown_selected)
    dropdown_row.add_child(search_dropdown)

    var inject_btn = Button.new()
    inject_btn.text = "Inject"
    inject_btn.pressed.connect(_on_inject_search_selected)
    dropdown_row.add_child(inject_btn)
    panel.add_child(dropdown_row)

    panel.add_child(HSeparator.new())

    var quick_label = Label.new()
    quick_label.text = "Quick Inject:"
    quick_label.add_theme_color_override("font_color", Color(0.8, 0.8, 0.8))
    panel.add_child(quick_label)

    var scroll = ScrollContainer.new()
    scroll.custom_minimum_size = Vector2(panel_width - 10, 300)
    scroll.size_flags_horizontal = Control.SIZE_FILL
    scroll.size_flags_vertical = Control.SIZE_FILL
    panel.add_child(scroll)

    button_container = VBoxContainer.new()
    button_container.size_flags_horizontal = Control.SIZE_FILL
    scroll.add_child(button_container)

    var top_n = mini(top_neuropil_count, neuropil_names.size())
    for i in range(top_n):
        _add_neuropil_button(neuropil_names[i], neuropil_vertex_counts[i])

    status_label = Label.new()
    status_label.text = "Ready"
    status_label.add_theme_color_override("font_color", Color(0.6, 0.8, 0.6))
    panel.add_child(status_label)


func _add_neuropil_button(name: String, count: int) -> void:
    var row = HBoxContainer.new()
    var btn = Button.new()
    btn.text = name
    btn.size_flags_horizontal = Control.SIZE_FILL
    btn.tooltip_text = "%s (%d neurons)" % [name, count]
    btn.pressed.connect(_on_neuropil_button_pressed.bind(name))
    row.add_child(btn)

    var cnt_label = Label.new()
    cnt_label.text = "%d" % count
    cnt_label.add_theme_color_override("font_color", Color(0.6, 0.6, 0.8))
    cnt_label.custom_minimum_size = Vector2(50, 0)
    cnt_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
    row.add_child(cnt_label)

    button_container.add_child(row)
    neuropil_buttons.append(btn)


func _on_neuropil_button_pressed(np_name: String) -> void:
    _inject_stimulus(np_name)


func _on_dropdown_selected(idx: int) -> void:
    if idx <= 0:
        return
    var np_name = search_dropdown.get_item_text(idx)
    _inject_stimulus(np_name)


func _on_inject_search_selected() -> void:
    var idx = search_dropdown.selected
    if idx > 0:
        var np_name = search_dropdown.get_item_text(idx)
        _inject_stimulus(np_name)


func _on_intensity_changed(value: float) -> void:
    intensity_label.text = "%d" % int(value)


func _on_search_changed(filter_text: String) -> void:
    search_dropdown.clear()
    search_dropdown.add_item("-- Select neuropil --")
    if filter_text.strip_edges().is_empty():
        for name in neuropil_names:
            search_dropdown.add_item(name)
    else:
        var ft = filter_text.to_upper()
        for name in neuropil_names:
            if ft in name.to_upper():
                search_dropdown.add_item(name)


func _inject_stimulus(np_name: String) -> void:
    if not blanket or not graph:
        return

    var verts = graph.get_vertices_by_neuropil(np_name)
    var intensity = int(intensity_slider.value)
    var count = verts.size()
    if count == 0:
        status_label.text = "No neurons in %s" % np_name
        return

    for vi in verts:
        if vi >= 0 and vi < blanket.get_neuron_count():
            blanket.inject_sensory(vi, float(intensity))

    last_injection_neuropil = np_name
    last_injection_intensity = intensity
    status_label.text = "Injected %.1f into %s (%d neurons)" % [intensity, np_name, count]
    stimulus_injected.emit(np_name, intensity)
    print("[Stimulus] %s <- %.1f (%d neurons)" % [np_name, intensity, count])
