# SESSION CHECKPOINT — Phase 5 Complete

## Current State (2026-05-31)

### Phase 4: Character Recognition — SOLID
- **97.9% accuracy** on 13,633-character corpus (~75s DQFR turbo training)
- 10→32→31 MLP (1375 params, Xavier init, lr=0.1, ReLU→softmax→cross-entropy)
- Injection signature bypass: 10-dimensional pseudorandom pattern per character, range 200-1000
- Communication demo: 100% character-perfect echo of "HELLO BRAIN", "HOW ARE YOU", "I AM LEARNING"

### Phase 5: Sequence Prediction — COMPLETE
- **Bigram GM**: 31 states × 31 observations — 18.0% top-1
- **Trigram GM**: 31 states × 961 observations — **40.4% top-1, 68.9% top-3**
- Combined (0.3×bi + 0.7×tri): 39.9% top-1, 66.4% top-3
- Text generation: Autoregressive character prediction with loop detection
- GM save/load: Persistent to `user://seq_gm.bin`
- Average VFE: 10.72 (prediction surprise metric, displayed during communication)

### Phase 6: Active Inference Loop — COMPLETE
- Injection intensity modulated by prediction confidence (0.3×-3.0× range)
- `prediction_mod = clamp(0.3 + (1.0 - confidence) * 3.0, 0.3, 3.0)`
- Predicted chars → weak injection (brain "knows" this)
- Surprising chars → strong injection (brain "attends" to this)
- Decoder reads unmodulated signatures → recognition accuracy unaffected (97.9%)
- VFE shown per-message: "HELLO BRAIN" VFE=8.3, "HOW ARE YOU" VFE=15.1, "I AM LEARNING" VFE=10.6
- Context reset per message (`prev_char_idx = -1`, `last_char_idx = -1`)

### C++ Changes (recompiled binary)
1. `generative_model.h/.cpp`: Added `get_likelihood`, `set_likelihood`, `get_prior`, `set_prior` bindings
2. `dqfr_controller.cpp`: Added `get_inference_ref()` binding (previously done)
3. Binary: `godot-engine/bin/godot.linuxbsd.editor.x86_64` — last recompiled with new GM bindings

### GDScript Components (language_interface.gd)
New functions added:
- `_init_sequence_gm()` — Initialize bigram + trigram GMs
- `_train_sequence(char_idx)` — Train both GMs on (prev,curr,next) triples
- `predict_next_char(curr)` / `predict_with_context(prev, curr)` — Combined prediction
- `get_combined_beliefs(prev, curr)` — Returns distribution over next char
- `generate_text(seed, max_chars)` — Autoregressive text generation with loop detection
- `evaluate_sequence_prediction()` — Full corpus evaluation (top-1, top-3, VFE)
- `save_sequence_gm()` / `load_sequence_gm()` — Binary persistence
- Corpus loading from external file `res://demo/training_corpus.txt` (13,633 chars)

### Key Files
- Authoritative: `modules/usf_engine/demo/language_interface.gd`
- Synced to: `godot-engine/modules/usf_engine/demo/language_interface.gd`
- Corpus: `godot-engine/demo/training_corpus.txt` (13,633 chars, Free Energy Principle text)
- Scene: `godot-engine/demo/bootstrap_demo.tscn` + `bootstrap_demo.gd`
- Docs: `modules/usf_engine/ARCHITECTURE.md` (updated), `EXPLAIN.md` (updated)
- C++ source: `modules/usf_engine/thermo/generative_model.h/.cpp`
- C++ source: `modules/usf_engine/dqfr/dqfr_controller.cpp`

### Build Command
```bash
cd godot-engine && scons -j$(nproc) module_usf_engine_enabled=yes target=editor platform=linuxbsd
```

### Run Command
```bash
cd godot-engine && timeout 300 ./bin/godot.linuxbsd.editor.x86_64 --path . --headless
```

### Phase 7: Word-Level Language Model — COMPLETE
- Word tokenizer: maps text → word IDs, vocabulary of 501 words (top 500 of 925 unique)
- Word bigram GM: 501 states × 501 observations (voice: 250K entries, ~2MB)
- Temperature sampling: `sample_from_distribution(beliefs, T)` replaces argmax
  - T=0.0 → argmax (original behavior)
  - T=0.5 → focused generation, follows learned patterns closely
  - T=1.0 → diverse generation, explores more associations
  - T=1.5+ → near-random (too much noise)
- Word generation demo with historical corpus:
  - `generate_word_sequence("THE", 12, 0.5)` → 'THE MANDELA MILLIONS EXPONENTIALLY JAZZ FLOURISHING...'
  - `generate_word_sequence("THE", 12, 1.0)` → 'THE AVAILABLE NEWS PRINTING ACROSS GALILEO TELESCOPE...'
  - `generate_word_sequence("BRAIN", 10, 0.8)` → 'BRAIN ONE WORTH BEGAN SURPRISE BENDS...'
- Save/load: `word_gm.bin`, `nn_weights.bin`, `seq_gm.bin` all persisted

### Current Results Summary (latest run)

| Metric | Value |
|---|---|
| Character recognition (MLP) | 98.0% |
| Char trigram top-1 | 32.5% |
| Word vocab size | 501 (top 500 of 925 unique) |
| Word pairs trained | 2,642 |
| Word generation | Working with temperature control |
| Active Inference VFE | 9.3-13.3 range |
| Communication demo | Perfect echo (3 messages) |
| Corpus | 15,544 chars historical text |
| Persistence | All 3 binaries saved/loaded |

### Architecture Changes (Phase 7)
New functions added to `language_interface.gd`:
- `_tokenize_words(text)` → Array of word strings
- `_build_word_vocab()` → builds freq-sorted vocabulary from corpus
- `_word_to_id(w)` / `_id_to_word(id)` → bidirectional mapping
- `_tokenize_corpus_to_ids()` → convert corpus to word ID array
- `_train_word_gm()` → train word bigram GM from corpus
- `predict_next_word(current_word_id)` → bigram prediction
- `save_word_gm()` / `load_word_gm()` → binary persistence
- `sample_from_distribution(beliefs, temperature)` → temperature-controlled sampling
- `generate_word_sequence(seed, max_words, temperature)` → full word generation

### Next Session — TODO
1. **Higher-order word n-grams**: Extend from bigram to trigram (different encoding to avoid O(V²) explosion — use compressed context or feature hashing)
2. **Larger historical corpus**: Expand to 100K+ chars from books, encyclopedias, or historical archives
3. **Word-level Active Inference**: Extend injection modulation to word prediction confidence
4. **Prediction UI**: Show top-3 next words in the dashboard during word generation
5. **Multi-epoch neural decoder training**: Run multiple epochs through character recognition (currently 1 epoch at 98.0%)
6. **Restore DQFR process_step**: Try using unfiltered firing rates again with the trained MLP
2. **Higher-order n-grams**: 4-gram model (31×31×31 = 29,791 observations) for better text generation
3. **Temperature sampling**: Add diversity to text generation (sample from distribution instead of argmax)
4. **Large corpus**: Download Project Gutenberg text or similar for dramatically better sequence learning
5. **Word-level prediction**: Move from characters to words once character-level accuracy is sufficient
6. **Neural decoder save/load**: Persist MLP weights so brain doesn't need retraining
7. **Prediction UI**: Show prediction bar in the dashboard (top-3 next characters with confidences)
8. **Multi-epoch training**: Run corpus through neural decoder multiple times (currently 1 epoch at 97.9%)
