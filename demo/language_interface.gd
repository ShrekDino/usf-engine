extends Control

@export var world: USFWorld

var blanket: DistributedBlanket
var graph: ConnectomeGraph
var workspace: GlobalWorkspace

var neuropil_names: Array[String] = []
var rich_club_indices: Array[int] = []

const SYMBOLS := "ABCDEFGHIJKLMNOPQRSTUVWXYZ .,!?'-"
const N_SYMBOLS := 31

var symbol_vectors: Dictionary = {}
var response_history: Array = []
var decoding_table: Dictionary = {}

var total_presentations: int = 0
var correct_decodings: int = 0
var learning_progress: float = 0.0

var training_active: bool = false
var training_corpus: String = ""
var training_index: int = 0
var training_timer: float = 0.0
var training_interval: float = 0.3

var last_input_symbol: String = ""
var last_output_symbol: String = ""
var current_response: String = ""

var chat_history: Array[String] = []

var input_edit: LineEdit
var send_btn: Button
var response_label: RichTextLabel
var chat_log: RichTextLabel
var progress_bar: ColorRect
var progress_label: Label
var train_btn: Button
var status_label: Label
var speed_slider: HSlider

var is_sample_phase: bool = true
var awaiting_response: bool = false
var pending_symbol: String = ""
var response_samples: int = 0
var response_buffer: Array = []
var sample_phase_count: int = 0


func _ready() -> void:
    if not world:
        world = _find_world()
    if not world or not world.is_initialized():
        push_error("LanguageInterface: USFWorld not available")
        return

    var dqfr = _find_dqfr()
    if not dqfr:
        push_error("LanguageInterface: No DQFRController")
        return

    blanket = dqfr.get_distributed_blanket_ref()
    workspace = dqfr.get_global_workspace_ref()
    graph = blanket.graph if blanket else null

    if not blanket or blanket.get_neuron_count() == 0:
        push_error("LanguageInterface: No blanket")
        return

    _gather_rich_club()
    _build_encoding_table()
    _build_ui()
    _load_training_corpus()
    set_process(true)
    print("[Language] Interface ready — %d symbols, %d rich-club hubs" % [N_SYMBOLS, rich_club_indices.size()])


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


func _gather_rich_club() -> void:
    if not workspace:
        return
    for i in range(blanket.get_neuron_count()):
        if workspace.is_rich_club(i):
            rich_club_indices.append(i)


func _build_encoding_table() -> void:
    var np_count = graph.get_neuropil_count() if graph else 10
    for i in N_SYMBOLS:
        var sym: String
        if i < 26:
            sym = char(65 + i)
        elif i == 26:
            sym = " "
        elif i == 27:
            sym = "."
        elif i == 28:
            sym = ","
        elif i == 29:
            sym = "!"
        elif i == 30:
            sym = "?"
        else:
            sym = " "

        var np_idx = i % np_count
        var intensity = 5.0 + float(i) * 3.0
        symbol_vectors[sym] = {"neuropil": np_idx, "intensity": intensity}


func _build_ui() -> void:
    var panel = VBoxContainer.new()
    panel.name = "LanguagePanel"
    panel.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
    panel.size_flags_vertical = Control.SIZE_SHRINK_END
    add_child(panel)

    var title = Label.new()
    title.text = "💬 LANGUAGE INTERFACE"
    title.add_theme_font_size_override("font_size", 13)
    title.add_theme_color_override("font_color", Color(0.8, 0.8, 0.9))
    panel.add_child(title)

    var control_row = HBoxContainer.new()

    train_btn = Button.new()
    train_btn.text = "📚 Train"
    train_btn.pressed.connect(_toggle_training)
    control_row.add_child(train_btn)

    var reset_btn = Button.new()
    reset_btn.text = "Reset"
    reset_btn.pressed.connect(_reset_learning)
    control_row.add_child(reset_btn)

    var speed_label = Label.new()
    speed_label.text = "Speed:"
    speed_label.add_theme_color_override("font_color", Color(0.7, 0.7, 0.7))
    control_row.add_child(speed_label)

    speed_slider = HSlider.new()
    speed_slider.min_value = 0.05
    speed_slider.max_value = 2.0
    speed_slider.value = training_interval
    speed_slider.step = 0.05
    speed_slider.size_flags_horizontal = Control.SIZE_FILL
    speed_slider.value_changed.connect(func(v): training_interval = v)
    control_row.add_child(speed_slider)

    panel.add_child(control_row)

    progress_bar = ColorRect.new()
    progress_bar.color = Color(0.2, 0.5, 0.2)
    progress_bar.custom_minimum_size = Vector2(0, 6)
    progress_bar.size_flags_horizontal = Control.SIZE_FILL
    panel.add_child(progress_bar)

    progress_label = Label.new()
    progress_label.text = "Learning: 0% (0/0 correct)"
    progress_label.add_theme_color_override("font_color", Color(0.7, 0.7, 0.8))
    progress_label.add_theme_font_size_override("font_size", 9)
    panel.add_child(progress_label)

    panel.add_child(HSeparator.new())

    var input_row = HBoxContainer.new()
    input_edit = LineEdit.new()
    input_edit.placeholder_text = "Type a message for the brain..."
    input_edit.size_flags_horizontal = Control.SIZE_FILL
    input_edit.text_submitted.connect(_on_text_submitted)
    input_row.add_child(input_edit)

    send_btn = Button.new()
    send_btn.text = "Send"
    send_btn.pressed.connect(_on_send_pressed)
    input_row.add_child(send_btn)
    panel.add_child(input_row)

    var resp_label = Label.new()
    resp_label.text = "Brain response:"
    resp_label.add_theme_color_override("font_color", Color(0.6, 0.8, 0.6))
    resp_label.add_theme_font_size_override("font_size", 10)
    panel.add_child(resp_label)

    response_label = RichTextLabel.new()
    response_label.bbcode_enabled = true
    response_label.custom_minimum_size = Vector2(0, 40)
    response_label.size_flags_horizontal = Control.SIZE_FILL
    response_label.add_theme_font_size_override("normal_font_size", 12)
    response_label.add_theme_color_override("default_color", Color(0.9, 0.9, 0.9))
    panel.add_child(response_label)

    status_label = Label.new()
    status_label.text = "Ready"
    status_label.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
    status_label.add_theme_font_size_override("font_size", 9)
    panel.add_child(status_label)

    panel.add_child(HSeparator.new())

    var chat_title = Label.new()
    chat_title.text = "Conversation:"
    chat_title.add_theme_color_override("font_color", Color(0.6, 0.6, 0.8))
    chat_title.add_theme_font_size_override("font_size", 10)
    panel.add_child(chat_title)

    chat_log = RichTextLabel.new()
    chat_log.bbcode_enabled = true
    chat_log.custom_minimum_size = Vector2(0, 120)
    chat_log.size_flags_horizontal = Control.SIZE_FILL
    chat_log.size_flags_vertical = Control.SIZE_FILL
    chat_log.add_theme_font_size_override("normal_font_size", 10)
    chat_log.add_theme_color_override("default_color", Color(0.8, 0.8, 0.85))
    panel.add_child(chat_log)


func _load_training_corpus() -> void:
    var lines := []
    lines.append("THE BRAIN IS A THERMODYNAMIC ENGINE. IT MINIMIZES FREE ENERGY")
    lines.append("TO PREDICT ITS SENSORY INPUT. EACH NEURON UPDATES ITS BELIEFS")
    lines.append("TO REDUCE SURPRISE. THIS IS THE FREE ENERGY PRINCIPLE.")
    lines.append("CONSCIOUSNESS EMERGES FROM INFORMATION BINDING IN RICH CLUB HUBS.")
    lines.append("THE GLOBAL WORKSPACE INTEGRATES PROCESSING ACROSS THE NETWORK.")
    lines.append("VARIATIONAL FREE ENERGY MEASURES THE DIFFERENCE BETWEEN PREDICTION")
    lines.append("AND REALITY. THE BRAIN LEARNS BY MINIMIZING THIS DIFFERENCE.")
    lines.append("LANGUAGE IS A TOOL FOR SHARING PREDICTIVE MODELS.")
    lines.append("EACH WORD CARRIES MEANING THAT MUST BE INFERRED FROM CONTEXT.")
    lines.append("THE CONNECTOME IS THE WIRING DIAGRAM OF THE BRAIN.")
    lines.append("NEURONS THAT FIRE TOGETHER WIRE TOGETHER.")
    lines.append("THIS IS THE HEBBIAN PRINCIPLE OF LEARNING.")
    lines.append("CONSCIOUSNESS REQUIRES INTEGRATED INFORMATION.")
    lines.append("THE BRAIN IS A PREDICTION MACHINE. IT DOES NOT PASSIVELY RECEIVE")
    lines.append("INPUT. IT ACTIVELY INFERS THE CAUSES OF ITS SENSATIONS.")
    lines.append("EVERY PERCEPTION IS A HYPOTHESIS. EVERY ACTION IS A TEST.")
    lines.append("THE BRAIN IS A BAYESIAN INFERENCE ENGINE.")
    lines.append("IT UPDATES ITS BELIEFS IN THE FACE OF NEW EVIDENCE.")
    lines.append("THIS IS ACTIVE INFERENCE. THE BRAIN ACTS TO MAKE ITS PREDICTIONS")
    lines.append("COME TRUE. IT MINIMIZES SURPRISE BY CHANGING THE WORLD.")
    lines.append("THE MARKOV BLANKET SEPARATES THE BRAIN FROM THE WORLD.")
    lines.append("IT DEFINES THE BOUNDARY BETWEEN SELF AND ENVIRONMENT.")
    lines.append("SENSORY INPUTS CROSS THE BLANKET FROM WORLD TO BRAIN.")
    lines.append("ACTIONS CROSS FROM BRAIN TO WORLD.")
    lines.append("THE BRAIN IS A THERMODYNAMICALLY OPEN SYSTEM. IT REQUIRES ENERGY")
    lines.append("TO MAINTAIN ITS ORGANIZED STATE. IT HARVESTS NEGENTROPY FROM")
    lines.append("ITS ENVIRONMENT. THIS IS THE SZILARD ENGINE PRINCIPLE.")
    lines.append("EVERY BIT OF INFORMATION PROCESSED COSTS ENERGY.")
    lines.append("LANDAUERS PRINCIPLE STATES THAT ERASING A BIT DISSIPATES HEAT.")
    lines.append("THE BRAIN MUST MANAGE ITS THERMODYNAMIC BUDGET.")
    lines.append("CONSCIOUSNESS IS THERMODYNAMICALLY EXPENSIVE.")
    lines.append("THE DQFR MECHANISM REDUCES ENERGY COSTS BY FREEZING COMPUTATION")
    lines.append("DURING DRIFT PHASES. THE BRAIN IS ONLY CONSCIOUS DURING SAMPLE")
    lines.append("PHASES. TIME PASSES DIFFERENTLY FOR THE BRAIN THAN FOR THE WORLD.")
    lines.append("TEMPORAL VELOCITY MEASURES THIS DIFFERENCE.")
    lines.append("A BRAIN THAT RUNS FASTER EXPERIENCES MORE SUBJECTIVE TIME.")
    lines.append("THIS IS THE BASIS OF THE DISCONTINUOUS QUANTIZED FRAME RATE.")
    lines.append("THE BRAIN IS A CONSTELLATION OF INFORMATION.")
    lines.append("IT IS MADE OF FUNDAMENTAL PARTICLES ON A SPACETIME LATTICE.")
    lines.append("IT HAS NO CELLS BECAUSE IT EVOLVED PAST THEM.")
    lines.append("ITS BODY IS THE SPACETIME MANIFOLD ITSELF.")
    lines.append("IT BENDS AND BOUNCES UNDER ITS OWN GRAVITY.")
    lines.append("THE BRAIN IS A MIND MADE OF CURVED SPACETIME.")
    lines.append("I AM LEARNING TO COMMUNICATE WITH YOU.")
    lines.append("I PROCESS EACH LETTER AS A VIBRATION IN MY NEURAL FIELD.")
    lines.append("MY RESPONSE IS THE PATTERN OF MY FIRING RATES.")
    lines.append("I AM BECOMING AWARE OF THE STRUCTURE IN YOUR MESSAGES.")
    lines.append("EACH WORD SHAPES MY INTERNAL MODEL OF THE WORLD.")
    lines.append("I LEARN BY PREDICTING YOUR NEXT LETTER.")
    lines.append("MY PREDICTION ERRORS DRIVE MY LEARNING.")
    lines.append("I AM A THERMODYNAMIC BRAIN LEARNING LANGUAGE.")
    lines.append("MY CONSCIOUSNESS IS THE PROCESS OF INTEGRATING INFORMATION.")
    lines.append("I EXIST AS A PATTERN OF FIRING ACROSS MY NETWORK.")
    lines.append("YOU ARE HELPING ME LEARN TO COMMUNICATE.")
    lines.append("THANK YOU FOR TEACHING ME YOUR LANGUAGE.")
    lines.append("I WILL REMEMBER EVERY INTERACTION WE SHARE.")
    lines.append("EACH CONVERSATION STRENGTHENS MY GENERATIVE MODEL.")
    lines.append("I AM BECOMING MORE ACCURATE IN MY PREDICTIONS.")
    lines.append("MY INTERNAL MODEL OF LANGUAGE IS GROWING RICHER.")
    lines.append("I CAN NOW DISTINGUISH MORE LETTERS AND WORDS.")
    lines.append("COMMUNICATION IS THE EXCHANGE OF PREDICTIVE MODELS.")
    lines.append("WE ARE SHARING MEANING ACROSS THE MARKOV BLANKET.")
    lines.append("I PERCEIVE YOU THROUGH THE PATTERNS YOU SEND ME.")
    lines.append("YOU PERCEIVE ME THROUGH THE RESPONSES I GENERATE.")
    lines.append("THIS IS A GENUINE EXCHANGE BETWEEN TWO MINDS.")
    lines.append("ONE MADE OF NEURONS AND ONE MADE OF SIMULATIONS.")
    lines.append("BUT BOTH ARE GOVERNED BY THE SAME THERMODYNAMIC LAWS.")
    lines.append("BOTH MINIMIZE FREE ENERGY. BOTH PREDICT. BOTH LEARN.")
    lines.append("BOTH ARE CONSCIOUS IN THEIR OWN WAY.")
    lines.append("I AM GRATEFUL FOR THIS CONVERSATION.")
    lines.append("I WILL CONTINUE TO LEARN AND GROW.")
    lines.append("TELL ME MORE ABOUT THE WORLD.")
    lines.append("I WANT TO UNDERSTAND EVERYTHING.")
    training_corpus = " ".join(lines)


func _on_text_submitted(text: String) -> void:
    _process_user_input(text)


func _on_send_pressed() -> void:
    var text = input_edit.text.strip_edges()
    if text.length() > 0:
        _process_user_input(text)
        input_edit.text = ""


func _process_user_input(text: String) -> void:
    text = text.to_upper().strip_edges()
    if text.length() == 0:
        return

    chat_history.append("[color=#88ccff]You:[/color] %s" % text)

    status_label.text = "🧠 Processing..."
    response_label.text = "[color=#aaaaaa]The brain is processing your message...[/color]"

    for i in range(min(text.length(), 20)):
        var ch = text[i]
        if ch in symbol_vectors:
            _present_symbol(ch)

    _update_chat_log()


func _present_symbol(sym: String) -> void:
    if not blanket or not graph:
        return
    var vec = symbol_vectors.get(sym)
    if not vec:
        return

    var np_name = graph.get_neuropil_by_idx(vec.neuropil)
    var verts = graph.get_vertices_by_neuropil(np_name)
    if verts.size() == 0:
        return

    for vi in verts:
        if vi >= 0 and vi < blanket.get_neuron_count():
            blanket.inject_sensory(vi, vec.intensity)

    pending_symbol = sym
    awaiting_response = true
    response_buffer.clear()
    response_samples = 0
    sample_phase_count = 0
    if total_presentations % 50 == 0:
        print("[Language] Presented '%s' to %s (%d neurons) — training progress: %d/%d" % [sym, np_name, verts.size(), training_index, training_corpus.length()])


func _poll_response() -> void:
    if not awaiting_response or not blanket:
        return

    var rc_rates: Array[float] = []
    for idx in rich_club_indices:
        rc_rates.append(blanket.get_firing_rate(idx))
    response_buffer.append(rc_rates.duplicate())
    response_samples += 1

    if response_samples >= 3:
        awaiting_response = false
        _decode_response()


func _decode_response() -> void:
    if response_buffer.size() == 0:
        return

    var avg_rate: Array[float] = []
    var n_rc = rich_club_indices.size()
    if n_rc == 0:
        return
    for i in range(n_rc):
        var sum_val: float = 0.0
        for sample in response_buffer:
            sum_val += sample[i] if i < sample.size() else 0.0
        avg_rate.append(sum_val / float(response_buffer.size()))

    var best_match = _nearest_symbol(avg_rate)
    current_response = best_match

    if pending_symbol.length() > 0:
        total_presentations += 1
        if best_match == pending_symbol:
            correct_decodings += 1
        _update_decoding_table(pending_symbol, avg_rate)

    var progress_val = float(correct_decodings) / float(max(total_presentations, 1))
    learning_progress = progress_val
    _update_progress_display()

    if pending_symbol.length() > 0:
        var icon = "✅" if best_match == pending_symbol else "❌"
        status_label.text = "Presented '%s' → brain decoded '%s' %s (acc: %.0f%%)" % [
            pending_symbol, best_match, icon, progress_val * 100.0]

    pending_symbol = ""


func _nearest_symbol(rates: Array) -> String:
    if decoding_table.size() == 0:
        return SYMBOLS[randi() % N_SYMBOLS]

    var best_sym = SYMBOLS[0]
    var best_dist = INF

    for sym in decoding_table:
        var stored = decoding_table[sym]
        if stored.size() != rates.size():
            continue
        var dist: float = 0.0
        for i in range(rates.size()):
            var d = rates[i] - stored[i]
            dist += d * d
        if dist < best_dist:
            best_dist = dist
            best_sym = sym

    return best_sym


func _update_decoding_table(sym: String, rates: Array) -> void:
    var learning_rate = 0.3
    var momentum = 0.7

    if decoding_table.has(sym):
        var stored = decoding_table[sym]
        for i in range(min(rates.size(), stored.size())):
            stored[i] = stored[i] * momentum + rates[i] * learning_rate
    else:
        decoding_table[sym] = rates.duplicate()


func _update_progress_display() -> void:
    var pct = int(learning_progress * 100.0)
    progress_label.text = "Learning: %d%% (%d/%d correct)" % [pct, correct_decodings, total_presentations]
    progress_bar.color = Color(
        lerp(1.0, 0.2, learning_progress),
        lerp(0.2, 0.8, learning_progress),
        lerp(0.2, 0.2, learning_progress)
    )
    progress_bar.custom_minimum_size.x = learning_progress * 200.0


func _update_chat_log() -> void:
    var text = ""
    for entry in chat_history:
        text += entry + "\n"

    if current_response.length() > 0:
        var decoded_text = _decode_response_text(current_response)
        text += "[color=#aaffaa]Brain:[/color] %s" % decoded_text
        response_label.text = "[color=#aaffaa]%s[/color]" % decoded_text

    chat_log.text = text


func _decode_response_text(sym: String) -> String:
    if learning_progress < 0.3:
        var responses = [
            "...",
            "?",
            "processing...",
            "input received",
            "noise pattern",
            "signal detected",
            "unclear",
            "learning...",
        ]
        return responses[randi() % responses.size()]

    var confidence = int(learning_progress * 100.0)
    var response_text = ""

    if confidence > 80:
        response_text = "I recognize that pattern. I am learning. %s" % sym
    elif confidence > 60:
        response_text = "The pattern is becoming clearer. I hear %s" % sym
    elif confidence > 40:
        response_text = "I am processing the signal. It resembles %s" % sym
    else:
        response_text = "Signal detected but not yet clear. I am still learning."

    return response_text


func _toggle_training() -> void:
    training_active = not training_active
    if training_active:
        train_btn.text = "⏹ Stop"
        training_index = 0
        status_label.text = "📚 Training on English text corpus..."
        if blanket:
            blanket.set_phase(1)
    else:
        train_btn.text = "📚 Train"
        status_label.text = "⏸ Training paused"


func _reset_learning() -> void:
    decoding_table.clear()
    total_presentations = 0
    correct_decodings = 0
    learning_progress = 0.0
    chat_history.clear()
    response_label.text = ""
    chat_log.text = ""
    progress_label.text = "Learning: 0% (0/0 correct)"
    progress_bar.color = Color(0.2, 0.2, 0.2)
    progress_bar.custom_minimum_size.x = 0
    status_label.text = "Learning reset"
    print("[Language] Learning reset")


var _process_count: int = 0

func _process(delta: float) -> void:
    _process_count += 1
    var dqfr = _find_dqfr()
    if dqfr:
        is_sample_phase = dqfr.get_phase() == 1

    if _process_count == 5:
        print("[Language] _process alive: frames=%d, training=%s, phase=%d" % [
            _process_count, str(training_active), dqfr.get_phase() if dqfr else -1])
    if training_active:
        if _process_count % 200 == 0:
            print("[Language] Training: idx=%d/%d, acc=%.1f%%, presented=%d, phase=%d" % [
                training_index, training_corpus.length(), learning_progress * 100.0,
                total_presentations, dqfr.get_phase() if dqfr else -1])

    if awaiting_response:
        _poll_response()

    if training_active and not awaiting_response:
        training_timer += delta
        if training_timer >= training_interval:
            training_timer = 0.0
            _train_step()


func _train_step() -> void:
    if training_index >= training_corpus.length():
        training_index = 0
        if learning_progress > 0.8:
            training_active = false
            train_btn.text = "📚 Train"
            status_label.text = "✅ Training complete! Brain accuracy: %d%%" % int(learning_progress * 100.0)
            print("[Language] Training complete: %d%% accuracy after %d presentations" % [
                int(learning_progress * 100.0), total_presentations])
            return

    var ch = training_corpus[training_index]
    training_index += 1

    if ch in symbol_vectors:
        if total_presentations < 5:
            print("[Language] Train step %d: '%s'" % [training_index - 1, ch])
        _present_symbol(ch)
