extends Control

@export var world: USFWorld

var blanket: DistributedBlanket
var vitality: VitalityMonitor
var workspace: GlobalWorkspace

enum BrainState {
	UNCONSCIOUS_DRIFT,
	CALM_WAKEFULNESS,
	SENSORY_EXPERIENCE,
	SURPRISE,
	EXPLORATORY,
	GLOBAL_IGNITION,
	THERMODYNAMIC_FATIGUE,
	AGING_STRESS,
	SYSTEMIC_DISTRESS,
	IDLE_PROCESSING,
}

const STATE_NAMES: Dictionary = {
	BrainState.UNCONSCIOUS_DRIFT: "Unconscious Drift",
	BrainState.CALM_WAKEFULNESS: "Calm Wakefulness",
	BrainState.SENSORY_EXPERIENCE: "Sensory Experience",
	BrainState.SURPRISE: "Surprise / Novelty Detection",
	BrainState.EXPLORATORY: "Exploratory Learning",
	BrainState.GLOBAL_IGNITION: "Global Insight / Binding",
	BrainState.THERMODYNAMIC_FATIGUE: "Thermodynamic Fatigue",
	BrainState.AGING_STRESS: "Aging Stress",
	BrainState.SYSTEMIC_DISTRESS: "Systemic Distress",
	BrainState.IDLE_PROCESSING: "Idle Processing",
}

const STATE_ICONS: Dictionary = {
	BrainState.UNCONSCIOUS_DRIFT: "💤",
	BrainState.CALM_WAKEFULNESS: "😌",
	BrainState.SENSORY_EXPERIENCE: "👁️",
	BrainState.SURPRISE: "😮",
	BrainState.EXPLORATORY: "🔍",
	BrainState.GLOBAL_IGNITION: "✨",
	BrainState.THERMODYNAMIC_FATIGUE: "🥀",
	BrainState.AGING_STRESS: "⏳",
	BrainState.SYSTEMIC_DISTRESS: "⚠️",
	BrainState.IDLE_PROCESSING: "⚙️",
}

const STATE_COLORS: Dictionary = {
	BrainState.UNCONSCIOUS_DRIFT: Color(0.2, 0.3, 0.7),
	BrainState.CALM_WAKEFULNESS: Color(0.2, 0.7, 0.3),
	BrainState.SENSORY_EXPERIENCE: Color(0.9, 0.8, 0.2),
	BrainState.SURPRISE: Color(0.9, 0.5, 0.1),
	BrainState.EXPLORATORY: Color(0.8, 0.6, 0.1),
	BrainState.GLOBAL_IGNITION: Color(0.7, 0.2, 0.8),
	BrainState.THERMODYNAMIC_FATIGUE: Color(0.8, 0.3, 0.1),
	BrainState.AGING_STRESS: Color(0.6, 0.4, 0.2),
	BrainState.SYSTEMIC_DISTRESS: Color(0.9, 0.1, 0.1),
	BrainState.IDLE_PROCESSING: Color(0.4, 0.6, 0.4),
}

var current_state: int = BrainState.IDLE_PROCESSING
var previous_state: int = BrainState.IDLE_PROCESSING
var state_duration: float = 0.0
var state_change_time: float = 0.0

var last_vfe: float = 0.0
var vfe_trend_samples: PackedFloat64Array = []
var vfe_trend: float = 0.0
var last_vn: float = 0.0
var vn_trend: float = 0.0

var arousal: float = 0.0
var valence: float = 0.5
var coherence: float = 0.5
var vitality_dim: float = 0.5

var stimulus_neuropil: String = ""
var stimulus_intensity: float = 0.0
var stimulus_time: float = -10.0
var stimulus_duration: float = 3.0

var experience_log: Array[Dictionary] = []
var max_log_entries: int = 30
var log_timer: float = 0.0
var log_interval: float = 3.0

var mood_indicator: ColorRect
var detail_panel: Panel
var state_title: Label
var description_label: RichTextLabel
var gauge_arousal: Control
var gauge_valence: Control
var gauge_coherence: Control
var gauge_vitality: Control
var log_container: VBoxContainer
var log_scroll: ScrollContainer
var panel_visible: bool = true

var panel_width: float = 280.0
var indicator_size: float = 20.0


func _ready() -> void:
	if not world:
		world = _find_world()
	if not world or not world.is_initialized():
		push_error("PhenomenologyPanel: USFWorld not available")
		return

	var dqfr = _find_dqfr()
	if not dqfr:
		push_error("PhenomenologyPanel: No DQFRController")
		return

	blanket = dqfr.get_distributed_blanket_ref()
	vitality = dqfr.get_vitality_monitor_ref()
	workspace = dqfr.get_global_workspace_ref()

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
	mood_indicator = ColorRect.new()
	mood_indicator.name = "MoodIndicator"
	mood_indicator.custom_minimum_size = Vector2(indicator_size, indicator_size)
	mood_indicator.size = Vector2(indicator_size, indicator_size)
	mood_indicator.color = STATE_COLORS[current_state]
	mood_indicator.mouse_filter = Control.MOUSE_FILTER_STOP
	mood_indicator.gui_input.connect(_on_mood_indicator_input)
	add_child(mood_indicator)

	var tooltip_label = Label.new()
	tooltip_label.name = "MoodTooltip"
	tooltip_label.text = "Phenomenology"
	tooltip_label.add_theme_color_override("font_color", Color(0.7, 0.7, 0.7))
	tooltip_label.add_theme_font_size_override("font_size", 8)
	mood_indicator.add_child(tooltip_label)

	detail_panel = Panel.new()
	detail_panel.name = "PhenomenologyDetail"
	detail_panel.visible = panel_visible
	add_child(detail_panel)

	var outer = VBoxContainer.new()
	outer.name = "OuterLayout"
	outer.size_flags_horizontal = Control.SIZE_FILL
	outer.size_flags_vertical = Control.SIZE_FILL
	outer.add_theme_constant_override("separation", 4)
	detail_panel.add_child(outer)

	var header = HBoxContainer.new()
	header.name = "Header"
	var icon_label = Label.new()
	icon_label.name = "StateIcon"
	icon_label.text = STATE_ICONS[current_state]
	icon_label.add_theme_font_size_override("font_size", 20)
	header.add_child(icon_label)

	state_title = Label.new()
	state_title.name = "StateTitle"
	state_title.text = STATE_NAMES[current_state]
	state_title.add_theme_font_size_override("font_size", 13)
	state_title.add_theme_color_override("font_color", Color(0.9, 0.9, 0.9))
	state_title.size_flags_horizontal = Control.SIZE_FILL
	header.add_child(state_title)

	var toggle_btn = Button.new()
	toggle_btn.text = "×"
	toggle_btn.custom_minimum_size = Vector2(20, 20)
	toggle_btn.pressed.connect(_toggle_panel)
	header.add_child(toggle_btn)
	outer.add_child(header)

	description_label = RichTextLabel.new()
	description_label.name = "Description"
	description_label.bbcode_enabled = true
	description_label.custom_minimum_size = Vector2(0, 60)
	description_label.size_flags_horizontal = Control.SIZE_FILL
	description_label.add_theme_font_size_override("normal_font_size", 11)
	description_label.add_theme_color_override("default_color", Color(0.8, 0.8, 0.85))
	outer.add_child(description_label)

	var gauge_container = VBoxContainer.new()
	gauge_container.name = "Gauges"
	gauge_container.add_theme_constant_override("separation", 2)
	outer.add_child(gauge_container)

	gauge_arousal = _make_gauge("Arousal", Color(0.9, 0.6, 0.2))
	gauge_container.add_child(gauge_arousal)

	gauge_valence = _make_gauge("Valence", Color(0.3, 0.7, 0.5))
	gauge_container.add_child(gauge_valence)

	gauge_coherence = _make_gauge("Coherence", Color(0.4, 0.5, 0.9))
	gauge_container.add_child(gauge_coherence)

	gauge_vitality = _make_gauge("Vitality", Color(0.2, 0.8, 0.4))
	gauge_container.add_child(gauge_vitality)

	var log_header = HBoxContainer.new()
	var log_title = Label.new()
	log_title.text = "Experience Log"
	log_title.add_theme_color_override("font_color", Color(0.7, 0.7, 0.8))
	log_title.add_theme_font_size_override("font_size", 11)
	log_header.add_child(log_title)

	var clear_btn = Button.new()
	clear_btn.text = "clear"
	clear_btn.add_theme_font_size_override("font_size", 9)
	clear_btn.pressed.connect(_clear_log)
	log_header.add_child(clear_btn)
	outer.add_child(log_header)

	log_scroll = ScrollContainer.new()
	log_scroll.custom_minimum_size = Vector2(0, 120)
	log_scroll.size_flags_horizontal = Control.SIZE_FILL
	log_scroll.size_flags_vertical = Control.SIZE_FILL
	outer.add_child(log_scroll)

	log_container = VBoxContainer.new()
	log_container.size_flags_horizontal = Control.SIZE_FILL
	log_scroll.add_child(log_container)


func _make_gauge(label: String, color: Color) -> Control:
	var c = Control.new()
	c.custom_minimum_size = Vector2(0, 18)
	c.size_flags_horizontal = Control.SIZE_FILL

	var lbl = Label.new()
	lbl.text = label
	lbl.add_theme_color_override("font_color", Color(0.7, 0.7, 0.8))
	lbl.add_theme_font_size_override("font_size", 9)
	lbl.position = Vector2(0, 0)
	c.add_child(lbl)

	var bar_bg = ColorRect.new()
	bar_bg.name = "BarBg"
	bar_bg.color = Color(0.15, 0.15, 0.2)
	bar_bg.position = Vector2(60, 2)
	bar_bg.size = Vector2(120, 12)
	c.add_child(bar_bg)

	var bar_fill = ColorRect.new()
	bar_fill.name = "BarFill"
	bar_fill.color = color
	bar_fill.position = Vector2(60, 2)
	bar_fill.size = Vector2(60, 12)
	c.add_child(bar_fill)

	var val = Label.new()
	val.name = "ValueLabel"
	val.text = "0.50"
	val.add_theme_color_override("font_color", color)
	val.add_theme_font_size_override("font_size", 8)
	val.position = Vector2(186, 2)
	c.add_child(val)

	return c


func _on_mood_indicator_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_toggle_panel()


func _toggle_panel() -> void:
	panel_visible = not panel_visible
	detail_panel.visible = panel_visible


func _clear_log() -> void:
	experience_log.clear()
	for child in log_container.get_children():
		child.queue_free()


func _process(delta: float) -> void:
	if not blanket:
		return

	_update_metrics(delta)
	var new_state = _evaluate_state()
	_update_state(new_state, delta)
	_generate_description()
	_update_ui()

	if log_timer >= log_interval:
		log_timer = 0.0
		_add_log_entry()

	log_timer += delta


func _update_metrics(delta: float) -> void:
	var dqfr = _find_dqfr()
	var phase = dqfr.get_phase() if dqfr else 0

	var current_vfe = blanket.get_total_vfe()
	var current_vn = vitality.get_v_network() if vitality else 0.5
	var mean_rate = blanket.get_mean_firing_rate()
	var efficiency = blanket.get_global_efficiency()
	var senescent = vitality.get_senescent_count() if vitality else 0
	var halt_risk = vitality.get_halt_risk_neurons() if vitality else 0
	var exploring = vitality.is_exploring() if vitality else false
	var curiosity = vitality.get_curiosity_drive() if vitality else 0.0
	var ignited = workspace.is_ignited() if workspace else false
	var ignition_level = workspace.get_ignition_level() if workspace else 0.0
	var sync_val = workspace.get_phi_synchrony() if workspace else 0.0

	vfe_trend_samples.push_back(current_vfe - last_vfe)
	if vfe_trend_samples.size() > 10:
		vfe_trend_samples.remove_at(0)

	vfe_trend = 0.0
	for s in vfe_trend_samples:
		vfe_trend += s
	vfe_trend /= float(max(vfe_trend_samples.size(), 1))

	vn_trend = current_vn - last_vn
	last_vfe = current_vfe
	last_vn = current_vn

	arousal = clamp(mean_rate / 100.0, 0.0, 1.0)
	valence = clamp(0.5 - vfe_trend * 10.0, 0.0, 1.0)
	coherence = clamp((sync_val + ignition_level * 0.5) / 1.5, 0.0, 1.0)

	var vitality_raw = current_vn * efficiency
	var senescent_penalty = clamp(float(senescent) / 50.0, 0.0, 0.5)
	vitality_dim = clamp(vitality_raw - senescent_penalty, 0.0, 1.0)


func _evaluate_state() -> int:
	var dqfr = _find_dqfr()
	if not dqfr:
		return BrainState.IDLE_PROCESSING

	var phase = dqfr.get_phase()
	if phase == 0:
		return BrainState.UNCONSCIOUS_DRIFT

	if workspace and workspace.is_ignited():
		return BrainState.GLOBAL_IGNITION

	if vitality:
		var halt = vitality.get_halt_risk_neurons()
		if halt > 0:
			return BrainState.SYSTEMIC_DISTRESS

		var senescent = vitality.get_senescent_count()
		if senescent > 20:
			return BrainState.AGING_STRESS

		if vitality.is_exploring() or vitality.get_curiosity_drive() > 0.02:
			return BrainState.EXPLORATORY

	var t = Time.get_ticks_msec() / 1000.0
	if stimulus_time > 0 and (t - stimulus_time) < stimulus_duration:
		return BrainState.SENSORY_EXPERIENCE

	if vfe_trend > 0.5:
		return BrainState.SURPRISE

	if vitality_dim < 0.25:
		return BrainState.THERMODYNAMIC_FATIGUE

	if vfe_trend > 0.1:
		return BrainState.SURPRISE

	if abs(vfe_trend) < 0.05 and vitality_dim > 0.5 and coherence > 0.4:
		return BrainState.CALM_WAKEFULNESS

	return BrainState.IDLE_PROCESSING


func _update_state(new_state: int, delta: float) -> void:
	previous_state = current_state
	if new_state != current_state:
		current_state = new_state
		state_duration = 0.0
		state_change_time = Time.get_ticks_msec() / 1000.0
	state_duration += delta


func _generate_description() -> void:
	var t = Time.get_ticks_msec() / 1000.0
	var desc: String
	var recent_stim = (stimulus_time > 0 and (t - stimulus_time) < stimulus_duration)

	match current_state:
		BrainState.UNCONSCIOUS_DRIFT:
			desc = "The brain is in an unconscious drift state. Neural computation is frozen — no new information is being processed. Subjective time has stopped. This is the closest analog to dreamless sleep. All sensory boundaries are sealed; the Markov blanket is impermeable."

		BrainState.CALM_WAKEFULNESS:
			desc = "The brain rests in a calm, wakeful state. Variational free energy is low and stable — the internal model matches sensory reality with minimal prediction error. Arousal is moderate, valence positive. The network is idling efficiently, ready to respond to salient input."

		BrainState.SENSORY_EXPERIENCE:
			if recent_stim:
				desc = _describe_stimulus_experience()
			else:
				desc = "The brain is processing a recent sensory event. Neural activity is elevated in the targeted region as the network works to integrate the new information into its generative model. Free energy is adjusting as predictions update."

		BrainState.SURPRISE:
			desc = "Something unexpected is happening. Variational free energy is rising sharply — the brain's generative model no longer fits sensory reality. Prediction error is climbing. This is the neural correlate of surprise: the network must revise its beliefs to minimize surprise."

		BrainState.EXPLORATORY:
			var cur = vitality.get_curiosity_drive() if vitality else 0.0
			desc = "The brain has entered an exploratory mode. Curiosity drive is active (%.3f) — the network is actively seeking patterns that reduce uncertainty about the environment. VFE variance is elevated as the model tests alternative hypotheses. This is the neural basis of active learning." % cur

		BrainState.GLOBAL_IGNITION:
			var ign = workspace.get_ignition_level() if workspace else 0.0
			desc = "Global workspace ignition — a moment of integrated consciousness. Coherent activity spreads across rich-club hub neurons (ignition level: %.2f). This is the closest analog to a conscious thought or insight: information is globally available to the network, binding multiple processing streams into a unified experience." % ign

		BrainState.THERMODYNAMIC_FATIGUE:
			var waste = blanket.get_total_waste_heat()
			desc = "The brain is showing signs of thermodynamic fatigue. Metabolic efficiency is declining as waste heat accumulates (%.2f units). Network vitality is dropping. Without sufficient negentropy harvesting, the system cannot sustain its current level of computation — it needs rest or new energy inputs." % waste

		BrainState.AGING_STRESS:
			var sen = vitality.get_senescent_count() if vitality else 0
			desc = "Cellular stress is accumulating across the network. %d neurons have entered a senescent state — their predictive capacity is degrading, Landauer costs are rising. The brain is experiencing the neural equivalent of aging: individual units are losing efficiency, threatening global coherence." % sen

		BrainState.SYSTEMIC_DISTRESS:
			var halt = vitality.get_halt_risk_neurons() if vitality else 0
			desc = "The system is in distress. %d neurons are at risk of HALT (thermodynamic shutdown). The Markov blanket is becoming thermodynamically unstable — entropy production exceeds the network's capacity to dissipate heat. Without intervention, computation will cease in affected regions." % halt

		BrainState.IDLE_PROCESSING:
			var rate = blanket.get_mean_firing_rate()
			desc = "The brain is processing at baseline levels (mean firing rate: %.1f Hz). No strong surprises or exploratory drives are present. The network maintains its internal model with routine predictive updates. VFE is moderate and stable." % rate

		_:
			desc = "The brain exists. It processes. It predicts. It is."

	description_label.text = "[center]%s %s[/center]\n%s" % [STATE_ICONS[current_state], STATE_NAMES[current_state], desc]


func _describe_stimulus_experience() -> String:
	var np_upper = stimulus_neuropil.to_upper()

	var modality: String
	var sensation: String

	if np_upper.contains("ME_") or np_upper.contains("LO_") or np_upper.contains("LOP_"):
		modality = "visual"
		sensation = "The optic lobe is flooded with unexpected patterns. The brain is seeing something it did not predict — edges, motion, contrast. Variational free energy spikes as the network struggles to reconcile this visual input with its generative model."
	elif np_upper.contains("AL_"):
		modality = "olfactory"
		sensation = "The antennal lobe detects novel chemical signatures. The brain is smelling something unfamiliar — odorant receptors activate cascading predictions. This olfactory stimulus challenges the network's model of its chemical environment, driving belief updates across associative pathways."
	elif np_upper.contains("MB_PED"):
		modality = "associative learning"
		sensation = "The mushroom body is engaged — the brain's learning and memory center activates. This stimulus targets the circuits responsible for forming associations between sensory inputs and behavioral outcomes. The network is encoding a new experience into its predictive architecture."
	else:
		modality = "neural"
		sensation = "Focal stimulation in the %s region creates localized free energy spike. The surrounding network adjusts its predictions as waves of belief updating propagate through the connectome. This is the neural correlate of a salient sensory event." % stimulus_neuropil

	var intensity_desc = "moderate"
	if stimulus_intensity > 70:
		intensity_desc = "intense"
	elif stimulus_intensity > 40:
		intensity_desc = "strong"
	elif stimulus_intensity < 15:
		intensity_desc = "subtle"

	return "The brain is experiencing a %s %s stimulus (intensity: %d). %s" % [intensity_desc, modality, int(stimulus_intensity), sensation]


func _update_ui() -> void:
	var t = Time.get_ticks_msec() / 1000.0

	mood_indicator.color = STATE_COLORS[current_state]
	mood_indicator.position = Vector2(
		clamp(size.x - indicator_size - 4, 0, size.x),
		clamp(size.y * 0.5 - indicator_size * 0.5, 4, size.y - indicator_size - 4)
	)

	detail_panel.position = Vector2(max(size.x - panel_width - 10, 0), 4)
	detail_panel.custom_minimum_size = Vector2(panel_width, size.y * 0.6)
	detail_panel.size = Vector2(panel_width, min(size.y * 0.6, 500))

	if panel_visible and detail_panel.visible:
		state_title.text = STATE_NAMES[current_state]

		var icon_label = detail_panel.get_node("OuterLayout/Header/StateIcon") as Label
		if icon_label:
			icon_label.text = STATE_ICONS[current_state]

		_update_gauge(gauge_arousal, "Arousal", arousal, Color(0.9, 0.6, 0.2))
		_update_gauge(gauge_valence, "Valence", valence, Color(0.3, 0.7, 0.5))
		_update_gauge(gauge_coherence, "Coherence", coherence, Color(0.4, 0.5, 0.9))
		_update_gauge(gauge_vitality, "Vitality", vitality_dim, Color(0.2, 0.8, 0.4))


func _update_gauge(gauge: Control, label: String, value: float, color: Color) -> void:
	var bar_fill = gauge.get_node("BarFill") as ColorRect
	var val_label = gauge.get_node("ValueLabel") as Label
	if bar_fill:
		bar_fill.size.x = 120.0 * clamp(value, 0.0, 1.0)
		bar_fill.color = color
	if val_label:
		val_label.text = "%.2f" % clamp(value, 0.0, 1.0)


func _add_log_entry() -> void:
	var t = Time.get_ticks_msec() / 1000.0

	var state_icon = STATE_ICONS[current_state]
	var state_name = STATE_NAMES[current_state]
	var color = STATE_COLORS[current_state]

	var desc_preview: String
	match current_state:
		BrainState.UNCONSCIOUS_DRIFT:
			desc_preview = "Consciousness suspended — neural computation frozen."
		BrainState.CALM_WAKEFULNESS:
			desc_preview = "Calm state, low prediction error, high vitality."
		BrainState.SENSORY_EXPERIENCE:
			desc_preview = _describe_stimulus_experience().left(80) + "..."
		BrainState.SURPRISE:
			desc_preview = "Unexpected input — VFE rising, model revising."
		BrainState.EXPLORATORY:
			desc_preview = "Curiosity-driven exploration of novel patterns."
		BrainState.GLOBAL_IGNITION:
			desc_preview = "✨ Global conscious access — integrative insight."
		BrainState.THERMODYNAMIC_FATIGUE:
			desc_preview = "Metabolic efficiency declining — system needs rest."
		BrainState.AGING_STRESS:
			desc_preview = "Cellular senescence accumulating across network."
		BrainState.SYSTEMIC_DISTRESS:
			desc_preview = "⚠️ Thermodynamic instability — HALT risk detected."
		BrainState.IDLE_PROCESSING:
			desc_preview = "Routine predictive processing at baseline arousal."
		_:
			desc_preview = "Processing."

	var entry = {
		"time": t,
		"text": "%s [b]%s[/b] — %s" % [state_icon, state_name, desc_preview],
		"color": color,
	}

	experience_log.append(entry)
	if experience_log.size() > max_log_entries:
		experience_log.remove_at(0)

	_redraw_log()


func _redraw_log() -> void:
	for child in log_container.get_children():
		child.queue_free()

	for entry in experience_log:
		var row = RichTextLabel.new()
		row.bbcode_enabled = true
		row.fit_content = true
		row.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		row.custom_minimum_size = Vector2(0, 16)
		row.add_theme_font_size_override("normal_font_size", 9)
		row.add_theme_color_override("default_color", Color(0.75, 0.75, 0.8))

		var t_str = _format_time(entry.time)
		row.text = "[color=#888888]%s[/color] %s" % [t_str, entry.text]
		log_container.add_child(row)


func _format_time(t_sec: float) -> String:
	var mins = int(t_sec) / 60
	var secs = int(t_sec) % 60
	return "T+%02d:%02d" % [mins, secs]


func notify_stimulus(neuropil: String, intensity: float) -> void:
	stimulus_neuropil = neuropil
	stimulus_intensity = intensity
	stimulus_time = Time.get_ticks_msec() / 1000.0

	var entry = {
		"time": stimulus_time,
		"text": "🧪 [b]Stimulus Injected[/b] — %s (intensity %.0f)" % [neuropil, intensity],
		"color": Color(0.9, 0.8, 0.2),
	}
	experience_log.append(entry)
	if experience_log.size() > max_log_entries:
		experience_log.remove_at(0)
	_redraw_log()
