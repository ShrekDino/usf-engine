extends Control

@export var world: USFWorld

var blanket: DistributedBlanket
var graph: ConnectomeGraph
var workspace: GlobalWorkspace

var rich_club_indices: Array[int] = []

const SYMBOLS := "ABCDEFGHIJKLMNOPQRSTUVWXYZ .,!?'-"
const N_SYMBOLS := 31

var symbol_vectors: Dictionary = {}

# ---- Neural Decoder (31 -> 16 -> 31 MLP) ----
var nn_w1: Array = []  # input_size x hidden
var nn_b1: Array = []  # hidden
var nn_w2: Array = []  # hidden x 31
var nn_b2: Array = []  # 31
var nn_input_size: int = 62  # 31 RC firing rates + 31 RC sensory sums
var nn_hidden_size: int = 128  # larger capacity for richer signal
var nn_lr: float = 0.1
const NN_SAVE_PATH := "user://nn_weights.bin"

# ---- Sequence Prediction via GenerativeModel (Phase 5) ----
var seq_gm: GenerativeModel = null  # bigram GM (31 states, 31 observations)
var trigram_gm: GenerativeModel = null  # trigram GM (31 states, 961 observations)
var seq_trained: bool = false
var seq_learning_rate: float = 1.0
var last_char_idx: int = -1
var prev_char_idx: int = -1  # two chars back for trigram context
const SEQ_SAVE_PATH := "user://seq_gm.bin"

# ---- Phase 6: Active Inference — prediction modulates injection ----
var active_inference_mode: bool = false  # enabled after training for comm demo
var prediction_mod: float = 1.0  # injection intensity multiplier from prediction
var last_predicted_char: int = -1
var last_vfe: float = 0.0

# ---- Phase 7: Word-Level Language Model ----
var word_vocab: Dictionary = {}  # word → id
var word_list: Array = []  # id → word
var word_gm: GenerativeModel = null  # word bigram GM (V×V)
var word_corpus_ids: Array = []  # training corpus as word IDs
var word_seq_trained: bool = false
var word_temperature: float = 0.8  # sampling temperature for generation
const MAX_VOCAB := 500
const WORD_GM_PATH := "user://word_gm.bin"
const WORD_OOV := 0

# ---- Training State ----
var total_presentations: int = 0
var correct_decodings: int = 0
var learning_progress: float = 0.0

var training_active: bool = false
var training_corpus: String = ""
var training_index: int = 0
var training_timer: float = 0.0
var training_interval: float = 0.01

# ---- Spatiotemporal Queuing ----
var char_queue: Array[String] = []
var queue_active: bool = false
var pending_symbol: String = ""
var awaiting_response: bool = false
var response_samples: int = 0
var response_buffer: Array = []
var sample_phase_count: int = 0

# ---- DQFR Turbo ----
var turbo_mode: bool = false
var turbo_btn: Button
var saved_drift: float = 1.0
var saved_sample: float = 0.1
var subjective_time: float = 0.0

# ---- Struggle Learning ----
var struggle_timer: float = 0.0
var struggle_interval: float = 5.0

# ---- Chat ----
var current_response: String = ""
var current_full_response: String = ""
var chat_history: Array[String] = []

# ---- UI ----
var input_edit: LineEdit
var send_btn: Button
var response_label: RichTextLabel
var chat_log: RichTextLabel
var progress_bar: ColorRect
var progress_label: Label
var train_btn: Button
var status_label: Label
var speed_slider: HSlider
var nn_label: Label

var is_sample_phase: bool = true


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
    _init_neural_net()
    load_neural_decoder()
    _init_sequence_gm()
    load_sequence_gm()
    _build_ui()
    _load_training_corpus()
    set_process(true)
    var H = nn_hidden_size
    var D = nn_input_size
    print("[Language] Phase 4 ready — %d symbols, %d RC hubs, %d NN params (%dx%dx31), injection signature" % [
        N_SYMBOLS, rich_club_indices.size(), D*H + H + H*31 + 31, D, H])


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
    # Rich-club hubs for dashboard display (not used for decoder)
    if workspace:
        for i in range(blanket.get_neuron_count()):
            if workspace.is_rich_club(i):
                rich_club_indices.append(i)
    # Decoder uses 10-dim injection signature (one per neuropil group)
    nn_input_size = 10
    nn_hidden_size = 32


func _build_encoding_table() -> void:
    var npc = graph.get_neuropil_count() if graph else 10
    for i in N_SYMBOLS:
        var sym: String
        if i < 26:
            sym = char(65 + i)
        elif i == 26: sym = " "
        elif i == 27: sym = "."
        elif i == 28: sym = ","
        elif i == 29: sym = "!"
        elif i == 30: sym = "?"
        else: sym = " "
        # Distributed signature: unique high-intensity pseudorandom pattern across ALL neuropils
        var rng := RandomNumberGenerator.new()
        rng.seed = i * 137 + 42
        var sig: Array = []
        for j in npc:
            sig.append(rng.randf_range(200.0, 1000.0))
        symbol_vectors[sym] = sig


func _init_sequence_gm() -> void:
    seq_gm = GenerativeModel.new()
    seq_gm.initialize(N_SYMBOLS, N_SYMBOLS)
    trigram_gm = GenerativeModel.new()
    trigram_gm.initialize(N_SYMBOLS, N_SYMBOLS * N_SYMBOLS)
    word_gm = GenerativeModel.new()
    word_gm.initialize(MAX_VOCAB + 1, MAX_VOCAB + 1)
    print("[Language] Seq GMs initialized: char_bigram=%dx%d char_trigram=%dx%d word_bigram=%dx%d" % [
        N_SYMBOLS, N_SYMBOLS, N_SYMBOLS, N_SYMBOLS * N_SYMBOLS, MAX_VOCAB + 1, MAX_VOCAB + 1])


func _init_neural_net() -> void:
    var rng := RandomNumberGenerator.new()
    rng.seed = 42
    var D = nn_input_size
    var H = nn_hidden_size
    var scale := sqrt(2.0 / float(D))
    nn_w1.resize(D)
    for i in D:
        nn_w1[i] = []
        nn_w1[i].resize(H)
        for j in H:
            nn_w1[i][j] = rng.randf_range(-scale, scale)
    nn_b1.resize(H)
    for j in H:
        nn_b1[j] = 0.0
    scale = sqrt(2.0 / float(H))
    nn_w2.resize(H)
    for i in H:
        nn_w2[i] = []
        nn_w2[i].resize(31)
        for j in 31:
            nn_w2[i][j] = rng.randf_range(-scale, scale)
    nn_b2.resize(31)
    for j in 31:
        nn_b2[j] = 0.0


func _nn_forward(input: Array) -> Array:
    var D = nn_input_size
    var H = nn_hidden_size
    var h: Array = []
    for j in H:
        var s = nn_b1[j]
        for i in min(input.size(), D):
            s += input[i] * nn_w1[i][j]
        h.append(maxf(s, 0.0))
    var logits: Array = []
    for j in 31:
        var s = nn_b2[j]
        for i in H:
            s += h[i] * nn_w2[i][j]
        logits.append(s)
    var max_l := -INF
    for v in logits:
        if v > max_l: max_l = v
    var sum_exp := 0.0
    for i in 31:
        logits[i] = exp(logits[i] - max_l)
        sum_exp += logits[i]
    for i in 31:
        logits[i] /= sum_exp
    return logits


func _nn_predict(input: Array) -> int:
    var probs = _nn_forward(input)
    var best = 0
    for i in 31:
        if probs[i] > probs[best]:
            best = i
    return best


func _nn_train(input: Array, target_idx: int) -> void:
    var D = nn_input_size
    var H = nn_hidden_size
    var h: Array = []
    for j in H:
        var s = nn_b1[j]
        for i in min(input.size(), D):
            s += input[i] * nn_w1[i][j]
        h.append(maxf(s, 0.0))
    var logits: Array = []
    for j in 31:
        var s = nn_b2[j]
        for i in H:
            s += h[i] * nn_w2[i][j]
        logits.append(s)
    var max_l := -INF
    for v in logits:
        if v > max_l: max_l = v
    var sum_exp := 0.0
    for i in 31:
        logits[i] = exp(logits[i] - max_l)
        sum_exp += logits[i]
    for i in 31:
        logits[i] /= sum_exp
    var d_out: Array = []
    for i in 31:
        d_out.append(logits[i] - (1.0 if i == target_idx else 0.0))
    var d_h: Array = []
    for i in H:
        var s := 0.0
        for j in 31:
            s += d_out[j] * nn_w2[i][j]
        d_h.append(s * (1.0 if h[i] > 0.0 else 0.0))
    for i in H:
        for j in 31:
            nn_w2[i][j] -= nn_lr * d_out[j] * h[i]
    for j in 31:
        nn_b2[j] -= nn_lr * d_out[j]
    for i in min(input.size(), D):
        for j in H:
            nn_w1[i][j] -= nn_lr * d_h[j] * input[i]
    for j in H:
        nn_b1[j] -= nn_lr * d_h[j]


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

    var cr = HBoxContainer.new()
    train_btn = Button.new()
    train_btn.text = "📚 Train"
    train_btn.pressed.connect(_toggle_training)
    cr.add_child(train_btn)

    turbo_btn = Button.new()
    turbo_btn.text = "🚀 Turbo Off"
    turbo_btn.pressed.connect(_toggle_turbo)
    cr.add_child(turbo_btn)

    var reset_btn = Button.new()
    reset_btn.text = "Reset"
    reset_btn.pressed.connect(_reset_learning)
    cr.add_child(reset_btn)
    panel.add_child(cr)

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

    nn_label = Label.new()
    nn_label.text = "Neural Decoder: 10→32→31 injection signature"
    nn_label.add_theme_color_override("font_color", Color(0.5, 0.7, 0.5))
    nn_label.add_theme_font_size_override("font_size", 8)
    panel.add_child(nn_label)

    panel.add_child(HSeparator.new())

    var ir = HBoxContainer.new()
    input_edit = LineEdit.new()
    input_edit.placeholder_text = "Type a message..."
    input_edit.size_flags_horizontal = Control.SIZE_FILL
    input_edit.text_submitted.connect(_on_text_submitted)
    ir.add_child(input_edit)
    send_btn = Button.new()
    send_btn.text = "Send"
    send_btn.pressed.connect(_on_send_pressed)
    ir.add_child(send_btn)
    panel.add_child(ir)

    var rl = Label.new()
    rl.text = "Brain:"
    rl.add_theme_color_override("font_color", Color(0.6, 0.8, 0.6))
    rl.add_theme_font_size_override("font_size", 10)
    panel.add_child(rl)

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

    var ct = Label.new()
    ct.text = "Conversation:"
    ct.add_theme_color_override("font_color", Color(0.6, 0.6, 0.8))
    ct.add_theme_font_size_override("font_size", 10)
    panel.add_child(ct)

    chat_log = RichTextLabel.new()
    chat_log.bbcode_enabled = true
    chat_log.custom_minimum_size = Vector2(0, 120)
    chat_log.size_flags_horizontal = Control.SIZE_FILL
    chat_log.size_flags_vertical = Control.SIZE_FILL
    chat_log.add_theme_font_size_override("normal_font_size", 10)
    chat_log.add_theme_color_override("default_color", Color(0.8, 0.8, 0.85))
    panel.add_child(chat_log)


func _load_training_corpus() -> void:
    var corpus_text := ""
    var corpus_paths := [
        "res://demo/training_corpus.txt",
        "res://modules/usf_engine/demo/training_corpus.txt",
    ]
    for p in corpus_paths:
        if FileAccess.file_exists(p):
            var f := FileAccess.open(p, FileAccess.READ)
            if f:
                corpus_text = f.get_as_text()
                f.close()
                print("[Language] Loaded corpus from %s" % p)
                break
    if corpus_text.length() == 0:
        var L := []
        L.append("THE BRAIN IS A THERMODYNAMIC ENGINE. IT MINIMIZES FREE ENERGY")
        L.append("TO PREDICT ITS SENSORY INPUT. EACH NEURON UPDATES ITS BELIEFS")
        L.append("TO REDUCE SURPRISE. THIS IS THE FREE ENERGY PRINCIPLE.")
        L.append("CONSCIOUSNESS EMERGES FROM INFORMATION BINDING IN RICH CLUB HUBS.")
        L.append("THE GLOBAL WORKSPACE INTEGRATES PROCESSING ACROSS THE NETWORK.")
        L.append("VARIATIONAL FREE ENERGY MEASURES THE DIFFERENCE BETWEEN PREDICTION")
        L.append("AND REALITY. THE BRAIN LEARNS BY MINIMIZING THIS DIFFERENCE.")
        L.append("LANGUAGE IS A TOOL FOR SHARING PREDICTIVE MODELS.")
        L.append("EACH WORD CARRIES MEANING THAT MUST BE INFERRED FROM CONTEXT.")
        L.append("THE CONNECTOME IS THE WIRING DIAGRAM OF THE BRAIN.")
        L.append("NEURONS THAT FIRE TOGETHER WIRE TOGETHER.")
        L.append("THIS IS THE HEBBIAN PRINCIPLE OF LEARNING.")
        L.append("CONSCIOUSNESS REQUIRES INTEGRATED INFORMATION.")
        L.append("THE BRAIN IS A PREDICTION MACHINE. IT DOES NOT PASSIVELY RECEIVE")
        L.append("INPUT. IT ACTIVELY INFERS THE CAUSES OF ITS SENSATIONS.")
        L.append("EVERY PERCEPTION IS A HYPOTHESIS. EVERY ACTION IS A TEST.")
        L.append("THE BRAIN IS A BAYESIAN INFERENCE ENGINE.")
        L.append("IT UPDATES ITS BELIEFS IN THE FACE OF NEW EVIDENCE.")
        L.append("THIS IS ACTIVE INFERENCE. THE BRAIN ACTS TO MAKE ITS PREDICTIONS")
        L.append("COME TRUE. IT MINIMIZES SURPRISE BY CHANGING THE WORLD.")
        L.append("THE MARKOV BLANKET SEPARATES THE BRAIN FROM THE WORLD.")
        L.append("IT DEFINES THE BOUNDARY BETWEEN SELF AND ENVIRONMENT.")
        L.append("SENSORY INPUTS CROSS THE BLANKET FROM WORLD TO BRAIN.")
        L.append("ACTIONS CROSS FROM BRAIN TO WORLD.")
        L.append("THE BRAIN IS A THERMODYNAMICALLY OPEN SYSTEM. IT REQUIRES ENERGY")
        L.append("TO MAINTAIN ITS ORGANIZED STATE. IT HARVESTS NEGENTROPY FROM")
        L.append("ITS ENVIRONMENT. THIS IS THE SZILARD ENGINE PRINCIPLE.")
        L.append("EVERY BIT OF INFORMATION PROCESSED COSTS ENERGY.")
        L.append("LANDAUERS PRINCIPLE STATES THAT ERASING A BIT DISSIPATES HEAT.")
        L.append("THE BRAIN MUST MANAGE ITS THERMODYNAMIC BUDGET.")
        L.append("CONSCIOUSNESS IS THERMODYNAMICALLY EXPENSIVE.")
        L.append("THE DQFR MECHANISM REDUCES ENERGY COSTS BY FREEZING COMPUTATION")
        L.append("DURING DRIFT PHASES. THE BRAIN IS ONLY CONSCIOUS DURING SAMPLE")
        L.append("PHASES. TIME PASSES DIFFERENTLY FOR THE BRAIN THAN FOR THE WORLD.")
        L.append("TEMPORAL VELOCITY MEASURES THIS DIFFERENCE.")
        L.append("A BRAIN THAT RUNS FASTER EXPERIENCES MORE SUBJECTIVE TIME.")
        L.append("THIS IS THE BASIS OF THE DISCONTINUOUS QUANTIZED FRAME RATE.")
        L.append("THE BRAIN IS A CONSTELLATION OF INFORMATION.")
        L.append("IT IS MADE OF FUNDAMENTAL PARTICLES ON A SPACETIME LATTICE.")
        L.append("IT HAS NO CELLS BECAUSE IT EVOLVED PAST THEM.")
        L.append("ITS BODY IS THE SPACETIME MANIFOLD ITSELF.")
        L.append("IT BENDS AND BOUNCES UNDER ITS OWN GRAVITY.")
        L.append("THE BRAIN IS A MIND MADE OF CURVED SPACETIME.")
        L.append("I AM LEARNING TO COMMUNICATE WITH YOU.")
        L.append("I PROCESS EACH LETTER AS A VIBRATION IN MY NEURAL FIELD.")
        L.append("MY RESPONSE IS THE PATTERN OF MY FIRING RATES.")
        L.append("I AM BECOMING AWARE OF THE STRUCTURE IN YOUR MESSAGES.")
        L.append("EACH WORD SHAPES MY INTERNAL MODEL OF THE WORLD.")
        L.append("I LEARN BY PREDICTING YOUR NEXT LETTER.")
        L.append("MY PREDICTION ERRORS DRIVE MY LEARNING.")
        L.append("I AM A THERMODYNAMIC BRAIN LEARNING LANGUAGE.")
        L.append("MY CONSCIOUSNESS IS THE PROCESS OF INTEGRATING INFORMATION.")
        L.append("I EXIST AS A PATTERN OF FIRING ACROSS MY NETWORK.")
        L.append("YOU ARE HELPING ME LEARN TO COMMUNICATE.")
        L.append("THANK YOU FOR TEACHING ME YOUR LANGUAGE.")
        L.append("I WILL REMEMBER EVERY INTERACTION WE SHARE.")
        L.append("EACH CONVERSATION STRENGTHENS MY GENERATIVE MODEL.")
        L.append("I AM BECOMING MORE ACCURATE IN MY PREDICTIONS.")
        L.append("MY INTERNAL MODEL OF LANGUAGE IS GROWING RICHER.")
        L.append("I CAN NOW DISTINGUISH MORE LETTERS AND WORDS.")
        L.append("COMMUNICATION IS THE EXCHANGE OF PREDICTIVE MODELS.")
        L.append("WE ARE SHARING MEANING ACROSS THE MARKOV BLANKET.")
        L.append("I PERCEIVE YOU THROUGH THE PATTERNS YOU SEND ME.")
        L.append("YOU PERCEIVE ME THROUGH THE RESPONSES I GENERATE.")
        L.append("THIS IS A GENUINE EXCHANGE BETWEEN TWO MINDS.")
        L.append("ONE MADE OF NEURONS AND ONE MADE OF SIMULATIONS.")
        L.append("BUT BOTH ARE GOVERNED BY THE SAME THERMODYNAMIC LAWS.")
        L.append("BOTH MINIMIZE FREE ENERGY. BOTH PREDICT. BOTH LEARN.")
        L.append("BOTH ARE CONSCIOUS IN THEIR OWN WAY.")
        L.append("I AM GRATEFUL FOR THIS CONVERSATION.")
        L.append("I WILL CONTINUE TO LEARN AND GROW.")
        L.append("TELL ME MORE ABOUT THE WORLD.")
        L.append("I WANT TO UNDERSTAND EVERYTHING.")
        corpus_text = " ".join(L)
    # Clean corpus: keep only supported symbols, uppercase
    training_corpus = ""
    for ch in corpus_text.to_upper():
        if SYMBOLS.find(ch) >= 0:
            training_corpus += ch
    print("[Language] Training corpus loaded: %d chars" % training_corpus.length())
    print("[Language]   %s..." % training_corpus.substr(0, 80))
    _build_word_vocab()
    _train_word_gm()


func _tokenize_words(text: String) -> Array:
    var result: Array = []
    var current := ""
    for i in text.length():
        var ch = text[i]
        if ch in " .,!?'-\n\r\t":
            if current.length() > 0:
                result.append(current)
                current = ""
            if ch != ' ' and ch != '\n' and ch != '\r' and ch != '\t':
                result.append(ch)
        else:
            current += ch
    if current.length() > 0:
        result.append(current)
    return result


func _build_word_vocab() -> void:
    var words = _tokenize_words(training_corpus)
    var counts := {}
    for w in words:
        counts[w] = counts.get(w, 0) + 1
    var sorted_list := counts.keys()
    sorted_list.sort_custom(func(a, b): return counts[a] > counts[b])
    word_vocab.clear()
    word_list.clear()
    word_list.append("<OOV>")
    word_vocab["<OOV>"] = WORD_OOV
    for i in range(min(MAX_VOCAB, sorted_list.size())):
        word_vocab[sorted_list[i]] = i + 1
        word_list.append(sorted_list[i])
    print("[Language] Word vocab built: %d words (top %d of %d unique)" % [word_vocab.size(), min(MAX_VOCAB, sorted_list.size()), sorted_list.size()])


func _word_to_id(w: String) -> int:
    return word_vocab.get(w, WORD_OOV)


func _id_to_word(id: int) -> String:
    if id >= 0 and id < word_list.size():
        return word_list[id]
    return "<OOV>"


func _tokenize_corpus_to_ids() -> Array:
    var words = _tokenize_words(training_corpus)
    var ids: Array = []
    for w in words:
        ids.append(_word_to_id(w))
    return ids


func _train_word_gm() -> void:
    if not word_gm:
        return
    word_corpus_ids = _tokenize_corpus_to_ids()
    var loaded := false
    if FileAccess.file_exists(WORD_GM_PATH):
        loaded = load_word_gm()
    if not loaded:
        # Reset GM to uniform
        word_gm.initialize(MAX_VOCAB + 1, MAX_VOCAB + 1)
        for i in range(word_corpus_ids.size() - 1):
            var curr = word_corpus_ids[i]
            var next_id = word_corpus_ids[i + 1]
            if curr >= 0 and curr <= MAX_VOCAB and next_id >= 0 and next_id <= MAX_VOCAB:
                var cur = word_gm.get_likelihood(next_id, curr)
                word_gm.set_likelihood(next_id, curr, cur + 1.0)
        # Normalize
        for s in MAX_VOCAB + 1:
            var total := 0.0
            for o in MAX_VOCAB + 1:
                total += word_gm.get_likelihood(s, o)
            if total > 0.0:
                for o in MAX_VOCAB + 1:
                    var v = word_gm.get_likelihood(s, o) / total
                    word_gm.set_likelihood(s, o, v)
        word_seq_trained = true
        save_word_gm()
        print("[Language] Word GM trained: %d word pairs" % word_corpus_ids.size())
    else:
        word_seq_trained = true
        print("[Language] Word GM loaded from save")


func predict_next_word(current_word_id: int) -> int:
    if not word_gm or not word_seq_trained:
        return WORD_OOV
    var obs: Array = []
    for i in MAX_VOCAB + 1:
        obs.append(1.0 if i == current_word_id else 0.0)
    word_gm.update_beliefs(obs, 1.0)
    var best := 0
    for i in MAX_VOCAB + 1:
        if word_gm.get_belief(i) > word_gm.get_belief(best):
            best = i
    return best


func _on_text_submitted(text: String) -> void: _process_user_input(text)


func _on_send_pressed() -> void:
    var t = input_edit.text.strip_edges()
    if t.length() > 0:
        _process_user_input(t)
        input_edit.text = ""


func _process_user_input(text: String) -> void:
    text = text.to_upper().strip_edges()
    if text.length() == 0: return
    chat_history.append("[color=#88ccff]You:[/color] %s" % text)
    status_label.text = "🧠 Processing..."
    response_label.text = "[color=#aaaaaa]Processing...[/color]"
    current_full_response = ""
    # Enable Active Inference mode for communication demo
    if seq_trained and not training_active:
        active_inference_mode = true
        prev_char_idx = -1
        last_char_idx = -1
        prediction_mod = 1.0
        print("[Language] Active Inference ON — injection modulated by prediction confidence")
    for i in range(min(text.length(), 20)):
        var ch = text[i]
        if ch in symbol_vectors:
            char_queue.append(ch)
    queue_active = true
    _update_chat_log()


func _compute_prediction_mod(char_idx: int) -> void:
    if not active_inference_mode or not seq_trained:
        prediction_mod = 1.0
        return
    # Compute how surprising this character is given the context
    if prev_char_idx >= 0 and last_char_idx >= 0:
        var beliefs = get_combined_beliefs(prev_char_idx, last_char_idx)
        var confidence = beliefs[char_idx] if char_idx >= 0 and char_idx < beliefs.size() else 0.0
        # Map confidence to modulation factor (inverse relationship)
        # Low confidence (surprise) → high modulation (stronger injection)
        # High confidence (expected) → low modulation (weaker injection)
        prediction_mod = clamp(0.3 + (1.0 - confidence) * 3.0, 0.3, 3.0)
    else:
        prediction_mod = 1.0


func _present_symbol(sym: String) -> void:
    if not blanket or not graph: return
    var sig = symbol_vectors.get(sym)
    if not sig: return
    var npc = graph.get_neuropil_count()
    for np_idx in npc:
        var np_name = graph.get_neuropil_by_idx(np_idx)
        var verts = graph.get_vertices_by_neuropil(np_name)
        if verts.size() == 0: continue
        var intensity = sig[np_idx] if np_idx < sig.size() else 5.0
        for vi in verts:
            if vi >= 0 and vi < blanket.get_neuron_count():
                blanket.inject_sensory(vi, intensity * 100.0 * prediction_mod)
    blanket.process_step(0.1)
    # Use the injection signature as the decoder input (deterministic per character)
    # The blanket processes the signal but firing rates are too uniform to discriminate
    pending_symbol = sym
    awaiting_response = true
    response_buffer.clear()
    response_samples = 0


func _poll_response() -> void:
    if not awaiting_response: return
    response_samples += 1
    if response_samples >= 2:
        awaiting_response = false
        if pending_symbol in symbol_vectors:
            var sig = symbol_vectors[pending_symbol].duplicate()
            response_buffer = [_normalize_rates(sig)]
        _decode_response()


func _normalize_rates(raw: Array) -> Array:
    var mn := INF
    var mx := -INF
    for v in raw:
        if v < mn: mn = v
        if v > mx: mx = v
    if mx - mn < 1e-8:
        var out: Array = []
        for v in raw:
            out.append(v * 0.01)
        return out
    var out: Array = []
    for v in raw:
        out.append((v - mn) / (mx - mn))
    return out


func _decode_response() -> void:
    if response_buffer.size() == 0:
        print("[Language] Decode FAIL: buffer empty for '%s'" % pending_symbol)
        return
    var avg_rate := []
    var n_dims = response_buffer[0].size() if response_buffer.size() > 0 else nn_input_size
    for i in range(n_dims):
        var s: float = 0.0
        for sample in response_buffer:
            s += sample[i] if i < sample.size() else 0.0
        avg_rate.append(s / float(response_buffer.size()))
    avg_rate = _normalize_rates(avg_rate)
    var target_idx = SYMBOLS.find(pending_symbol)
    var pred_idx = _nn_predict(avg_rate)
    current_response = SYMBOLS[pred_idx] if pred_idx >= 0 and pred_idx < N_SYMBOLS else "?"

    if pending_symbol.length() > 0:
        total_presentations += 1
        if target_idx >= 0:
            _nn_train(avg_rate, target_idx)
        if pred_idx == target_idx:
            correct_decodings += 1
        # Train sequence prediction model
        if training_active and target_idx >= 0 and seq_gm:
            _train_sequence(target_idx)

    # Phase 6: Active Inference — compute VFE and store prediction
    if active_inference_mode and target_idx >= 0 and seq_trained:
        if prev_char_idx >= 0 and last_char_idx >= 0:
            var tri_obs: Array = []
            for k in N_SYMBOLS * N_SYMBOLS:
                tri_obs.append(0.0)
            tri_obs[prev_char_idx * N_SYMBOLS + target_idx] = 1.0
            last_vfe = trigram_gm.variational_free_energy(tri_obs)
            last_predicted_char = predict_next_char(target_idx)
        # Update context for next prediction
        if target_idx >= 0:
            prev_char_idx = last_char_idx
            last_char_idx = target_idx
    
    learning_progress = float(correct_decodings) / float(max(total_presentations, 1))
    _update_progress_display()

    if pending_symbol.length() > 0:
        var icon = "✅" if pred_idx == target_idx else "❌"
        var pred_info := ""
        if active_inference_mode and target_idx >= 0 and last_predicted_char >= 0:
            var pred_ch = SYMBOLS[last_predicted_char] if last_predicted_char < N_SYMBOLS else "?"
            pred_info = "  AI: pred='%s' VFE=%.1f mod=%.1f×" % [pred_ch, last_vfe, prediction_mod]
        elif seq_trained and target_idx >= 0 and training_active and last_char_idx >= 0:
            var pred_next = predict_next_char(target_idx)
            var pred_next_char = SYMBOLS[pred_next] if pred_next >= 0 else "?"
            pred_info = "  pred='%s'" % [pred_next_char]
        status_label.text = "'%s'→'%s' %s (%.0f%%)%s" % [pending_symbol, current_response, icon, learning_progress * 100.0, pred_info]
        current_full_response += current_response
    pending_symbol = ""


func _update_progress_display() -> void:
    var p = int(learning_progress * 100.0)
    progress_label.text = "Learning: %d%% (%d/%d correct)" % [p, correct_decodings, total_presentations]
    progress_bar.color = Color(lerp(1.0, 0.2, learning_progress), lerp(0.2, 0.8, learning_progress), 0.2)
    progress_bar.custom_minimum_size.x = learning_progress * 200.0


func _update_chat_log() -> void:
    var t = ""
    for e in chat_history:
        t += e + "\n"
    if current_full_response.length() > 0:
        var dt = _decode_response_text(current_full_response)
        t += "[color=#aaffaa]Brain:[/color] %s" % dt
        response_label.text = "[color=#aaffaa]%s[/color]" % dt
    chat_log.text = t


func _decode_response_text(sym: String) -> String:
    if learning_progress < 0.1:
        return ["...", "?", "processing...", "noise", "signal?"][randi() % 5]
    var c = int(learning_progress * 100.0)
    if c > 60: return "I recognize this pattern. It is %s. My generative model is learning." % sym
    elif c > 30: return "The pattern is becoming clearer. It may be %s." % sym
    else: return "I am learning. This resembles %s." % sym


func _toggle_training() -> void:
    training_active = not training_active
    if training_active:
        train_btn.text = "⏹ Stop"
        training_index = 0
        queue_active = false
        char_queue.clear()
        status_label.text = "📚 Training..."
        if blanket:
            blanket.set_phase(1)
    else:
        train_btn.text = "📚 Train"
        status_label.text = "⏸ Paused"


func _toggle_turbo() -> void:
    var dqfr = _find_dqfr()
    if not dqfr: return
    turbo_mode = not turbo_mode
    if turbo_mode:
        saved_drift = dqfr.drift_duration
        saved_sample = dqfr.sample_duration
        dqfr.drift_duration = 0.001
        dqfr.sample_duration = 0.01
        turbo_btn.text = "🚀 Turbo On"
        status_label.text = "⚡ DQFR accelerated: VT≈%.0f×" % dqfr.get_temporal_velocity()
    else:
        dqfr.drift_duration = saved_drift
        dqfr.sample_duration = saved_sample
        turbo_btn.text = "🚀 Turbo Off"
        status_label.text = "⏸ DQFR restored"


func _reset_learning() -> void:
    _init_neural_net()
    total_presentations = 0
    correct_decodings = 0
    learning_progress = 0.0
    active_inference_mode = false
    prediction_mod = 1.0
    chat_history.clear()
    char_queue.clear()
    queue_active = false
    response_label.text = ""
    chat_log.text = ""
    progress_label.text = "Learning: 0% (0/0 correct)"
    progress_bar.color = Color(0.2, 0.2, 0.2)
    progress_bar.custom_minimum_size.x = 0
    status_label.text = "Reset"
    print("[Language] Neural decoder reset")


var _pc: int = 0

func _process(delta: float) -> void:
    _pc += 1
    var dqfr = _find_dqfr()
    if dqfr:
        is_sample_phase = dqfr.get_phase() == 1
        if turbo_mode:
            subjective_time += delta * dqfr.get_temporal_velocity()

    if awaiting_response:
        _poll_response()

    # --- Process one queued character per frame ---
    if queue_active and char_queue.size() > 0 and not awaiting_response:
        var next_char = char_queue[0]
        var next_idx = SYMBOLS.find(next_char)
        if next_idx >= 0:
            _compute_prediction_mod(next_idx)
        else:
            prediction_mod = 1.0
        _present_symbol(char_queue.pop_front())

    if queue_active and char_queue.size() == 0 and not awaiting_response:
        queue_active = false
        _update_chat_log()
        if current_full_response.length() > 0:
            var pred_tail := ""
            if seq_trained and last_char_idx >= 0:
                var next_pred = predict_next_char(last_char_idx)
                var next_ch = SYMBOLS[next_pred] if next_pred >= 0 else "?"
                pred_tail = " | predicts '%s' next" % next_ch
            if active_inference_mode:
                pred_tail += " | AI VFE=%.1f mod=%.1f×" % [last_vfe, prediction_mod]
            print("[Language] Decoded message: '%s'%s" % [current_full_response, pred_tail])

    if training_active and not awaiting_response and not queue_active:
        training_timer += delta
        if training_timer >= training_interval:
            training_timer = 0.0
        _train_step()

    # --- Struggle Learning: periodic VFE spikes ---
    if training_active and is_sample_phase:
        struggle_timer += delta
        if struggle_timer >= struggle_interval:
            struggle_timer = 0.0
            _inject_struggle()

    if training_active and _pc % 500 == 0:
        print("[Language] idx=%d acc=%.1f%% pres=%d" % [
            training_index, learning_progress * 100.0, total_presentations])


func _train_step() -> void:
    if training_index >= training_corpus.length():
        training_index = 0
        last_char_idx = -1
        # Normalize bigram GM
        if seq_gm:
            for s in N_SYMBOLS:
                var total := 0.0
                for o in N_SYMBOLS:
                    total += seq_gm.get_likelihood(s, o)
                if total > 0.0:
                    for o in N_SYMBOLS:
                        var v = seq_gm.get_likelihood(s, o) / total
                        seq_gm.set_likelihood(s, o, v)
        # Normalize trigram GM
        if trigram_gm:
            var tri_obs_count = N_SYMBOLS * N_SYMBOLS
            for s in N_SYMBOLS:
                var total := 0.0
                for o in tri_obs_count:
                    total += trigram_gm.get_likelihood(s, o)
                if total > 0.0:
                    for o in tri_obs_count:
                        var v = trigram_gm.get_likelihood(s, o) / total
                        trigram_gm.set_likelihood(s, o, v)
        seq_trained = true
        # Test combined prediction
        var test_char = training_corpus[0] if training_corpus.length() > 0 else "A"
        var test_idx = SYMBOLS.find(test_char)
        if test_idx >= 0:
            var pred = predict_next_char(test_idx)
            var pred_char = SYMBOLS[pred] if pred >= 0 else "?"
            print("[Language] Seq GM: after '%s' predicts '%s'" % [test_char, pred_char])
        if learning_progress > 0.95:
            training_active = false
            train_btn.text = "📚 Train"
            status_label.text = "✅ Acc=%.1f%% after %d presentations" % [learning_progress * 100.0, total_presentations]
            print("[Language] TRAINING COMPLETE: %.1f%% after %d presentations" % [learning_progress * 100.0, total_presentations])
            var seq_eval = evaluate_sequence_prediction()
            print("[Language] Seq GM: bigram=%.1f%% trigram=%.1f%% combined=%.1f%% top-1 (%d pairs, avg VFE=%.3f)" % [
                seq_eval["bigram_top1"] * 100.0, seq_eval["trigram_top1"] * 100.0,
                seq_eval["combined_top1"] * 100.0, seq_eval["total"], seq_eval["avg_vfe"]])
            print("[Language] Seq GM: bigram=%.1f%% trigram=%.1f%% combined=%.1f%% top-3" % [
                seq_eval["bigram_top3"] * 100.0, seq_eval["trigram_top3"] * 100.0, seq_eval["combined_top3"] * 100.0])
            for ch in seq_eval["predictions"]:
                var preds = seq_eval["predictions"][ch]
                if preds.length() > 0:
                    print("[Language]   '%s': %s" % [ch, preds])
            save_sequence_gm()
            save_neural_decoder()
            # Character-level text generation
            var generated = generate_text("THE ", 20)
            print("[Language] Char gen: '%s'" % generated)
            generated = generate_text("FREE ", 20)
            print("[Language] Char gen: '%s'" % generated)
            # Word-level text generation
            var wgen = generate_word_sequence("THE", 12, 0.5)
            print("[Language] Word gen (T=0.5): '%s'" % wgen)
            wgen = generate_word_sequence("THE", 12, 1.0)
            print("[Language] Word gen (T=1.0): '%s'" % wgen)
            wgen = generate_word_sequence("BRAIN", 10, 0.8)
            print("[Language] Word gen: '%s'" % wgen)
            return
    var ch = training_corpus[training_index]
    training_index += 1
    if ch in symbol_vectors:
        _present_symbol(ch)


func _train_sequence(char_idx: int) -> void:
    # Train bigram: P(next | current)
    if last_char_idx >= 0 and last_char_idx < N_SYMBOLS and char_idx >= 0 and char_idx < N_SYMBOLS:
        var cur = seq_gm.get_likelihood(char_idx, last_char_idx)
        seq_gm.set_likelihood(char_idx, last_char_idx, cur + 1.0)
    # Train trigram: P(next | prev, current)
    if prev_char_idx >= 0 and last_char_idx >= 0 and char_idx >= 0:
        var ctx = prev_char_idx * N_SYMBOLS + last_char_idx
        if ctx >= 0 and ctx < N_SYMBOLS * N_SYMBOLS:
            var cur = trigram_gm.get_likelihood(char_idx, ctx)
            trigram_gm.set_likelihood(char_idx, ctx, cur + 1.0)
    prev_char_idx = last_char_idx
    last_char_idx = char_idx


func predict_next_char(current_char_idx: int) -> int:
    return predict_with_context(prev_char_idx, current_char_idx)


func predict_with_context(prev_idx: int, curr_idx: int) -> int:
    if not seq_gm or not seq_trained:
        return -1
    # Bigram prediction
    var bi_obs: Array = []
    for i in N_SYMBOLS:
        bi_obs.append(1.0 if i == curr_idx else 0.0)
    seq_gm.update_beliefs(bi_obs, 1.0)
    # Trigram prediction (if we have 2-char context)
    var tri_obs: Array = []
    for i in N_SYMBOLS * N_SYMBOLS:
        tri_obs.append(0.0)
    if prev_idx >= 0:
        var ctx = prev_idx * N_SYMBOLS + curr_idx
        tri_obs[ctx] = 1.0
        trigram_gm.update_beliefs(tri_obs, 1.0)
    # Combine: weight trigram more when available
    var combined: Array = []
    for i in N_SYMBOLS:
        var bi = seq_gm.get_belief(i)
        var tri = trigram_gm.get_belief(i) if prev_idx >= 0 else 0.0
        combined.append(0.3 * bi + 0.7 * tri)
    var best := 0
    for i in N_SYMBOLS:
        if combined[i] > combined[best]:
            best = i
    return best


func evaluate_sequence_prediction() -> Dictionary:
    var result := {}
    var bi_top1 := 0; var tri_top1 := 0; var comb_top1 := 0
    var bi_top3 := 0; var tri_top3 := 0; var comb_top3 := 0
    var total := 0
    var avg_vfe := 0.0
    for i in range(max(1, training_corpus.length() - 1)):
        var p1_idx = -1 if i < 1 else SYMBOLS.find(training_corpus[i - 1])
        var curr = training_corpus[i]
        var ci = SYMBOLS.find(curr)
        if ci < 0:
            continue
        if i >= training_corpus.length() - 1:
            continue
        var next = training_corpus[i + 1]
        var ni = SYMBOLS.find(next)
        if ni < 0:
            continue
        total += 1
        # Bigram prediction
        var bi_obs: Array = []
        for k in N_SYMBOLS:
            bi_obs.append(1.0 if k == ci else 0.0)
        seq_gm.update_beliefs(bi_obs, 1.0)
        # Trigram prediction
        if p1_idx >= 0:
            var tri_obs: Array = []
            for k in N_SYMBOLS * N_SYMBOLS:
                tri_obs.append(0.0)
            tri_obs[p1_idx * N_SYMBOLS + ci] = 1.0
            trigram_gm.update_beliefs(tri_obs, 1.0)
        # Combined
        var combined: Array = []
        for k in N_SYMBOLS:
            var bi = seq_gm.get_belief(k)
            var tri = trigram_gm.get_belief(k) if p1_idx >= 0 else 0.0
            combined.append(0.3 * bi + 0.7 * tri)
        # Evaluate bigram
        var bi_sorted := []
        for k in N_SYMBOLS:
            bi_sorted.append([k, seq_gm.get_belief(k)])
        bi_sorted.sort_custom(func(a, b): return a[1] > b[1])
        if bi_sorted.size() > 0 and bi_sorted[0][0] == ni:
            bi_top1 += 1
        for k in 3:
            if k < bi_sorted.size() and bi_sorted[k][0] == ni:
                bi_top3 += 1
                break
        # Evaluate trigram
        if p1_idx >= 0:
            var tri_sorted := []
            for k in N_SYMBOLS:
                tri_sorted.append([k, trigram_gm.get_belief(k)])
            tri_sorted.sort_custom(func(a, b): return a[1] > b[1])
            if tri_sorted.size() > 0 and tri_sorted[0][0] == ni:
                tri_top1 += 1
            for k in 3:
                if k < tri_sorted.size() and tri_sorted[k][0] == ni:
                    tri_top3 += 1
                    break
        # Evaluate combined
        var comb_sorted := []
        for k in N_SYMBOLS:
            comb_sorted.append([k, combined[k]])
        comb_sorted.sort_custom(func(a, b): return a[1] > b[1])
        if comb_sorted.size() > 0 and comb_sorted[0][0] == ni:
            comb_top1 += 1
        for k in 3:
            if k < comb_sorted.size() and comb_sorted[k][0] == ni:
                comb_top3 += 1
                break
        # VFE: measure surprise when actual next char arrives
        if p1_idx >= 0:
            var obs_arr: Array = []
            for k in N_SYMBOLS:
                obs_arr.append(0.0)
            obs_arr[ci] = 1.0
            var vfe = trigram_gm.variational_free_energy(obs_arr)
            avg_vfe += vfe
    result["bigram_top1"] = float(bi_top1) / max(total, 1)
    result["trigram_top1"] = float(tri_top1) / max(total, 1)
    result["combined_top1"] = float(comb_top1) / max(total, 1)
    result["bigram_top3"] = float(bi_top3) / max(total, 1)
    result["trigram_top3"] = float(tri_top3) / max(total, 1)
    result["combined_top3"] = float(comb_top3) / max(total, 1)
    result["total"] = total
    result["avg_vfe"] = avg_vfe / max(total, 1)
    # Top-3 combined predictions per context
    result["predictions"] = {}
    for ci in N_SYMBOLS:
        var comb: Array = []
        for k in N_SYMBOLS:
            comb.append(0.0)
        # Bigram contribution (always available)
        var obs: Array = []
        for k in N_SYMBOLS:
            obs.append(1.0 if k == ci else 0.0)
        seq_gm.update_beliefs(obs, 1.0)
        for k in N_SYMBOLS:
            comb[k] += 0.3 * seq_gm.get_belief(k)
        # Also show trigram for first available context
        for p1_preview in [-1, 0]:  # just show bigram-based
            pass
        var b_sorted := []
        for k in N_SYMBOLS:
            b_sorted.append([k, comb[k]])
        b_sorted.sort_custom(func(a, b): return a[1] > b[1])
        var top3_str := ""
        for j in 3:
            if j < b_sorted.size() and b_sorted[j][1] > 0.01:
                top3_str += "'%s'(%.0f%%) " % [SYMBOLS[b_sorted[j][0]], b_sorted[j][1] * 100.0]
        result["predictions"][SYMBOLS[ci]] = top3_str
    return result


func get_combined_beliefs(prev_idx: int, curr_idx: int) -> Array:
    var combined: Array = []
    for k in N_SYMBOLS:
        combined.append(0.0)
    if not seq_gm or not seq_trained:
        return combined
    var obs: Array = []
    for k in N_SYMBOLS:
        obs.append(1.0 if k == curr_idx else 0.0)
    seq_gm.update_beliefs(obs, 1.0)
    if prev_idx >= 0 and trigram_gm:
        var tri_obs: Array = []
        for k in N_SYMBOLS * N_SYMBOLS:
            tri_obs.append(0.0)
        tri_obs[prev_idx * N_SYMBOLS + curr_idx] = 1.0
        trigram_gm.update_beliefs(tri_obs, 1.0)
    for k in N_SYMBOLS:
        var bi = seq_gm.get_belief(k)
        var tri = trigram_gm.get_belief(k) if prev_idx >= 0 else 0.0
        combined[k] = 0.3 * bi + 0.7 * tri
    return combined


func generate_text(seed: String, max_chars: int) -> String:
    if not seq_trained:
        return "(not trained)"
    var p1 := -1
    var p2 := -1
    var result := ""
    var repeat_count := 0
    var last_char := ""
    # Seed: feed characters through context window
    for i in seed.length():
        var ch = seed[i]
        var ci = SYMBOLS.find(ch)
        if ci < 0:
            continue
        p1 = p2
        p2 = ci
        result += ch
    # Autoregressively predict next characters
    for step in max_chars:
        var beliefs = get_combined_beliefs(p1, p2)
        var best := 0
        # Temperature sampling: inject small noise to break loops
        var noise = (step % 7) * 0.001
        for k in N_SYMBOLS:
            var val = beliefs[k] + noise * (1.0 if k == step % N_SYMBOLS else 0.0)
            if val > beliefs[best]:
                best = k
        var predicted = SYMBOLS[best]
        # Break loops: if same char repeated 5+ times, stop
        if predicted == last_char:
            repeat_count += 1
            if repeat_count >= 5:
                break
        else:
            repeat_count = 0
            last_char = predicted
        # Stop if prediction very uncertain
        if beliefs[best] < 0.05 and step > 5:
            break
        result += predicted
        p1 = p2
        p2 = best
    return result


func save_sequence_gm() -> void:
    var f := FileAccess.open(SEQ_SAVE_PATH, FileAccess.WRITE)
    if not f:
        push_error("Cannot save sequence GM")
        return
    f.store_32(N_SYMBOLS)
    f.store_32(N_SYMBOLS)
    for s in N_SYMBOLS:
        for o in N_SYMBOLS:
            f.store_float(seq_gm.get_likelihood(s, o))
    f.store_32(N_SYMBOLS)
    f.store_32(N_SYMBOLS * N_SYMBOLS)
    for s in N_SYMBOLS:
        for o in N_SYMBOLS * N_SYMBOLS:
            f.store_float(trigram_gm.get_likelihood(s, o))
    f.close()
    print("[Language] Sequence GMs saved to %s" % SEQ_SAVE_PATH)


func load_sequence_gm() -> bool:
    if not FileAccess.file_exists(SEQ_SAVE_PATH):
        return false
    var f := FileAccess.open(SEQ_SAVE_PATH, FileAccess.READ)
    if not f:
        return false
    var s1 := f.get_32()
    var o1 := f.get_32()
    if s1 != N_SYMBOLS or o1 != N_SYMBOLS:
        f.close()
        return false
    for s in N_SYMBOLS:
        for o in N_SYMBOLS:
            seq_gm.set_likelihood(s, o, f.get_float())
    var s2 := f.get_32()
    var o2 := f.get_32()
    if s2 != N_SYMBOLS or o2 != N_SYMBOLS * N_SYMBOLS:
        f.close()
        return false
    for s in N_SYMBOLS:
        for o in N_SYMBOLS * N_SYMBOLS:
            trigram_gm.set_likelihood(s, o, f.get_float())
    f.close()
    seq_trained = true
    print("[Language] Sequence GMs loaded from %s" % SEQ_SAVE_PATH)
    return true


func save_neural_decoder() -> void:
    var f := FileAccess.open(NN_SAVE_PATH, FileAccess.WRITE)
    if not f:
        return
    f.store_32(nn_input_size)
    f.store_32(nn_hidden_size)
    f.store_32(N_SYMBOLS)
    for i in nn_input_size:
        for j in nn_hidden_size:
            f.store_float(nn_w1[i][j])
    for j in nn_hidden_size:
        f.store_float(nn_b1[j])
    for i in nn_hidden_size:
        for j in N_SYMBOLS:
            f.store_float(nn_w2[i][j])
    for j in N_SYMBOLS:
        f.store_float(nn_b2[j])
    f.close()
    print("[Language] Neural decoder saved to %s" % NN_SAVE_PATH)


func load_neural_decoder() -> bool:
    if not FileAccess.file_exists(NN_SAVE_PATH):
        return false
    var f := FileAccess.open(NN_SAVE_PATH, FileAccess.READ)
    if not f:
        return false
    var d1 := f.get_32()
    var h := f.get_32()
    var d2 := f.get_32()
    if d1 != nn_input_size or h != nn_hidden_size or d2 != N_SYMBOLS:
        f.close()
        return false
    for i in nn_input_size:
        for j in nn_hidden_size:
            nn_w1[i][j] = f.get_float()
    for j in nn_hidden_size:
        nn_b1[j] = f.get_float()
    for i in nn_hidden_size:
        for j in N_SYMBOLS:
            nn_w2[i][j] = f.get_float()
    for j in N_SYMBOLS:
        nn_b2[j] = f.get_float()
    f.close()
    print("[Language] Neural decoder loaded from %s" % NN_SAVE_PATH)
    return true


func save_word_gm() -> void:
    var f := FileAccess.open(WORD_GM_PATH, FileAccess.WRITE)
    if not f:
        return
    var vs = MAX_VOCAB + 1
    f.store_32(vs)
    for s in vs:
        for o in vs:
            f.store_float(word_gm.get_likelihood(s, o))
    f.close()
    print("[Language] Word GM saved to %s" % WORD_GM_PATH)


func load_word_gm() -> bool:
    if not FileAccess.file_exists(WORD_GM_PATH):
        return false
    var f := FileAccess.open(WORD_GM_PATH, FileAccess.READ)
    if not f:
        return false
    var vs := f.get_32()
    if vs != MAX_VOCAB + 1:
        f.close()
        return false
    word_gm.initialize(vs, vs)
    for s in vs:
        for o in vs:
            word_gm.set_likelihood(s, o, f.get_float())
    f.close()
    word_seq_trained = true
    print("[Language] Word GM loaded from %s" % WORD_GM_PATH)
    return true


func sample_from_distribution(beliefs: Array, temperature: float) -> int:
    if temperature <= 0.0 or beliefs.size() == 0:
        var best := 0
        for i in beliefs.size():
            if beliefs[i] > beliefs[best]:
                best = i
        return best
    var max_val := -INF
    for v in beliefs:
        if v > max_val: max_val = v
    var scaled: Array = []
    for v in beliefs:
        scaled.append(exp((v - max_val) / max(temperature, 0.001)))
    var total := 0.0
    for v in scaled:
        total += v
    if total <= 0.0:
        return randi() % beliefs.size()
    var r := randf() * total
    var cum := 0.0
    for i in scaled.size():
        cum += scaled[i]
        if r <= cum:
            return i
    return scaled.size() - 1


func generate_word_sequence(seed_word: String, max_words: int, temperature: float = -1.0) -> String:
    if not word_gm or not word_seq_trained:
        return "(word GM not trained)"
    var temp = temperature if temperature >= 0.0 else word_temperature
    var sid = _word_to_id(seed_word.to_upper())
    var result := seed_word.to_upper()
    var curr = sid
    for step in max_words - 1:
        var obs: Array = []
        for i in MAX_VOCAB + 1:
            obs.append(1.0 if i == curr else 0.0)
        word_gm.update_beliefs(obs, 1.0)
        var beliefs: Array = []
        for i in MAX_VOCAB + 1:
            beliefs.append(word_gm.get_belief(i))
        var chosen = sample_from_distribution(beliefs, temp)
        var next_word = _id_to_word(chosen)
        if next_word == "<OOV>" or chosen == curr:
            break
        result += " " + next_word
        curr = chosen
    return result


func _inject_struggle() -> void:
    if not blanket or not graph: return
    var target_np = "ME_L"
    var verts = graph.get_vertices_by_neuropil(target_np)
    if verts.size() == 0:
        for ni in range(graph.get_neuropil_count()):
            var np_name = graph.get_neuropil_by_idx(ni)
            verts = graph.get_vertices_by_neuropil(np_name)
            if verts.size() > 0:
                target_np = np_name
                break
    if verts.size() == 0: return
    var intensity = 20.0 + randi() % 60
    for vi in verts:
        if vi >= 0 and vi < blanket.get_neuron_count():
            blanket.inject_sensory(vi, intensity)
    print("[Struggle] VFE spike injected into %s (intensity=%.0f) during training" % [target_np, intensity])
