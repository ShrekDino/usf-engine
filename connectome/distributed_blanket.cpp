#include "distributed_blanket.h"
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"
#include <cmath>

DistributedBlanket::DistributedBlanket() {}

void DistributedBlanket::set_graph(Ref<ConnectomeGraph> p_graph) {
    graph = p_graph;
}

void DistributedBlanket::initialize() {
    if (graph.is_null() || graph->get_vertex_count() == 0) {
        print_line("[Blanket] ERROR: No graph set");
        return;
    }

    int n = graph->get_vertex_count();
    states.resize(n);

    for (int i = 0; i < n; i++) {
        states.write[i].membrane_potential = -65.0;
        states.write[i].firing_rate = 0.0;
        states.write[i].energy_reserve = 1.0;
        states.write[i].prediction_error = 0.0;
        states.write[i].negentropy = 100.0;
        states.write[i].waste_heat = 0.0;
        states.write[i].temperature = 300.0;
        states.write[i].efficiency = 0.99;
        states.write[i].bits_processed = 0.0;
        states.write[i].landauer_cost_total = 0.0;
        states.write[i].negentropy_harvested = 0.0;
        states.write[i].belief_entropy = 0.0;
        states.write[i].sensory_sum = 0.0;
        states.write[i].sensory_count = 0.0;
        states.write[i].active_output = 0.0;
    }

    total_vfe = 0.0;
    total_negentropy = (double)n * 100.0;
    total_waste_heat = 0.0;
    total_landauer_cost = 0.0;
    total_negentropy_harvested = 0.0;
    mean_firing_rate = 0.0;
    mean_prediction_error = 0.0;
    global_efficiency = 0.0;
    active_neurons = n;
    current_phase = 0;
    sample_timer = 0.0;

    print_line("[Blanket] Initialized " + itos(n) + " neurons");
}

void DistributedBlanket::set_phase(int p) {
    current_phase = p;
}

double DistributedBlanket::sigmoid(double x) const {
    return 1.0 / (1.0 + std::exp(-x));
}

void DistributedBlanket::compute_sensory_input(int idx) {
    if (graph.is_null()) return;
    double sum = 0.0;
    double count = 0.0;

    int n_in = graph->get_incoming_count(idx);
    for (int i = 0; i < n_in; i++) {
        int ei = graph->get_incoming_edge(idx, i);
        int pre_idx = graph->get_edge_pre(ei);
        if (pre_idx >= 0 && pre_idx < states.size()) {
            double weight = (double)graph->get_edge_synapse_count(ei);
            sum += states[pre_idx].active_output * weight;
            count += weight;
        }
    }

    states.write[idx].sensory_sum = sum;
    states.write[idx].sensory_count = count > 0.0 ? count : 1.0;
}

void DistributedBlanket::compute_active_output(int idx) {
    double v = states[idx].membrane_potential;
    double rate = sigmoid((v + 30.0) / 10.0) * 200.0;
    states.write[idx].firing_rate = rate;
    states.write[idx].active_output = rate / 200.0;
}

void DistributedBlanket::update_metabolism(int idx, double dt) {
    NeuronState &s = states.write[idx];
    double k_B = 1.380649e-23;

    double spikes = s.firing_rate * dt;
    double cost = k_B * s.temperature * M_LN2 * spikes;
    s.energy_reserve -= cost;
    s.landauer_cost_total += cost;

    // Negentropy extraction (negentropy from sensory structure)
    if (s.sensory_count > 0.0) {
        double info_gain = std::log(1.0 + s.sensory_sum / s.sensory_count);
        s.negentropy_harvested += info_gain;
        s.negentropy -= cost + info_gain;
        s.waste_heat += cost * (1.0 - s.efficiency);
    }

    // Replenish from negentropy reserve
    if (s.energy_reserve < 0.1 && s.negentropy > 0.0) {
        double replenish = (s.negentropy * 0.01 < 0.1) ? (s.negentropy * 0.01) : 0.1;
        s.energy_reserve += replenish;
        s.negentropy -= replenish;
    }

    if (s.energy_reserve < 0.0) s.energy_reserve = 0.0;
    if (s.negentropy < 0.0) s.negentropy = 0.0;
    if (s.membrane_potential < -80.0) s.membrane_potential = -80.0;
    if (s.membrane_potential > 50.0) s.membrane_potential = 50.0;
}

void DistributedBlanket::update_landauer(int idx, double bits) {
    NeuronState &s = states.write[idx];
    double k_B = 1.380649e-23;
    double min_energy = k_B * s.temperature * M_LN2 * bits;
    s.bits_processed += bits;
    s.landauer_cost_total += min_energy;
    total_landauer_cost += min_energy;
}

void DistributedBlanket::update_beliefs(int idx, double dt) {
    NeuronState &s = states.write[idx];

    double kl_div = 0.0;
    if (s.firing_rate > 0.0) {
        double q = s.firing_rate / 200.0;
        double p = 0.01;
        kl_div = q * std::log(q / p) + (1.0 - q) * std::log((1.0 - q) / (1.0 - p));
    }

    double pred_err = s.sensory_sum;
    s.prediction_error = pred_err * pred_err;

    if (s.firing_rate > 0.0) {
        double q = s.firing_rate / 200.0;
        s.belief_entropy = -(q * std::log(q) + (1.0 - q) * std::log(1.0 - q + 1e-30));
    }

    double sig_prime = s.firing_rate * (1.0 - s.firing_rate / 200.0) / 200.0;
    double grad_vfe = pred_err * sig_prime + 0.01 * (s.membrane_potential + 65.0);
    s.membrane_potential -= 0.1 * grad_vfe * dt;

    if (s.membrane_potential < -80.0) s.membrane_potential = -80.0;
    if (s.membrane_potential > 50.0) s.membrane_potential = 50.0;

    total_vfe += kl_div + s.prediction_error;
}

void DistributedBlanket::aggregate_stats() {
    int n = states.size();
    if (n == 0) return;

    double total_firing = 0.0;
    double total_pred_err = 0.0;
    total_negentropy = 0.0;
    total_waste_heat = 0.0;
    total_landauer_cost = 0.0;
    total_negentropy_harvested = 0.0;
    active_neurons = 0;

    for (int i = 0; i < n; i++) {
        total_firing += states[i].firing_rate;
        total_pred_err += states[i].prediction_error;
        total_negentropy += states[i].negentropy;
        total_waste_heat += states[i].waste_heat;
        total_landauer_cost += states[i].landauer_cost_total;
        total_negentropy_harvested += states[i].negentropy_harvested;
        if (states[i].firing_rate > 0.5) active_neurons++;
    }

    mean_firing_rate = total_firing / (double)n;
    mean_prediction_error = total_pred_err / (double)n;

    if (total_landauer_cost > 0.0) {
        global_efficiency = total_negentropy_harvested / total_landauer_cost;
    } else {
        global_efficiency = 0.0;
    }
}

void DistributedBlanket::process_drift(double dt) {
    // Drift phase: blanket sealed, no computation, no S_gen
    // Preserve geometric state only — freeze all neuron states
    sample_timer += dt;
}

void DistributedBlanket::process_sample(double dt) {
    int n = states.size();
    if (n == 0 || graph.is_null()) return;

    // Phase 1: Sensory input from pre-synaptic partners
    for (int i = 0; i < n; i++) {
        compute_sensory_input(i);
    }

    // Phase 2: VFE gradient descent (belief update)
    for (int i = 0; i < n; i++) {
        update_beliefs(i, dt);
    }

    // Phase 3: Compute active output
    for (int i = 0; i < n; i++) {
        compute_active_output(i);
    }

    // Phase 4: Metabolic update + Landauer accounting
    for (int i = 0; i < n; i++) {
        update_metabolism(i, dt);
        double bits = states[i].firing_rate * dt * 0.001;
        update_landauer(i, bits);
    }

    // Phase 5: Aggregate global statistics
    aggregate_stats();

    sample_timer += dt;
}

void DistributedBlanket::process_step(double dt) {
    if (current_phase == 0) {
        process_drift(dt);
    } else {
        process_sample(dt);
    }
}

void DistributedBlanket::reset() {
    states.clear();
    total_vfe = 0.0;
    total_negentropy = 0.0;
    total_waste_heat = 0.0;
    total_landauer_cost = 0.0;
    total_negentropy_harvested = 0.0;
    mean_firing_rate = 0.0;
    mean_prediction_error = 0.0;
    global_efficiency = 0.0;
    active_neurons = 0;
    current_phase = 0;
    sample_timer = 0.0;
}

double DistributedBlanket::get_membrane_potential(int idx) const {
    return (idx >= 0 && idx < states.size()) ? states[idx].membrane_potential : 0.0;
}

double DistributedBlanket::get_firing_rate(int idx) const {
    return (idx >= 0 && idx < states.size()) ? states[idx].firing_rate : 0.0;
}

double DistributedBlanket::get_negentropy(int idx) const {
    return (idx >= 0 && idx < states.size()) ? states[idx].negentropy : 0.0;
}

double DistributedBlanket::get_prediction_error(int idx) const {
    return (idx >= 0 && idx < states.size()) ? states[idx].prediction_error : 0.0;
}

void DistributedBlanket::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_graph", "graph"), &DistributedBlanket::set_graph);
    ClassDB::bind_method(D_METHOD("get_graph"), &DistributedBlanket::get_graph);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "graph", PROPERTY_HINT_RESOURCE_TYPE, "ConnectomeGraph"), "set_graph", "get_graph");

    ClassDB::bind_method(D_METHOD("initialize"), &DistributedBlanket::initialize);
    ClassDB::bind_method(D_METHOD("process_step", "dt"), &DistributedBlanket::process_step);
    ClassDB::bind_method(D_METHOD("process_drift", "dt"), &DistributedBlanket::process_drift);
    ClassDB::bind_method(D_METHOD("process_sample", "dt"), &DistributedBlanket::process_sample);
    ClassDB::bind_method(D_METHOD("reset"), &DistributedBlanket::reset);

    ClassDB::bind_method(D_METHOD("set_phase", "phase"), &DistributedBlanket::set_phase);
    ClassDB::bind_method(D_METHOD("get_phase"), &DistributedBlanket::get_phase);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "phase", PROPERTY_HINT_ENUM, "Drift,Sample"), "set_phase", "get_phase");

    ClassDB::bind_method(D_METHOD("get_membrane_potential", "idx"), &DistributedBlanket::get_membrane_potential);
    ClassDB::bind_method(D_METHOD("get_firing_rate", "idx"), &DistributedBlanket::get_firing_rate);
    ClassDB::bind_method(D_METHOD("get_negentropy", "idx"), &DistributedBlanket::get_negentropy);
    ClassDB::bind_method(D_METHOD("get_prediction_error", "idx"), &DistributedBlanket::get_prediction_error);

    ClassDB::bind_method(D_METHOD("get_total_vfe"), &DistributedBlanket::get_total_vfe);
    ClassDB::bind_method(D_METHOD("get_total_negentropy"), &DistributedBlanket::get_total_negentropy);
    ClassDB::bind_method(D_METHOD("get_total_waste_heat"), &DistributedBlanket::get_total_waste_heat);
    ClassDB::bind_method(D_METHOD("get_total_landauer_cost"), &DistributedBlanket::get_total_landauer_cost);
    ClassDB::bind_method(D_METHOD("get_total_negentropy_harvested"), &DistributedBlanket::get_total_negentropy_harvested);
    ClassDB::bind_method(D_METHOD("get_global_efficiency"), &DistributedBlanket::get_global_efficiency);
    ClassDB::bind_method(D_METHOD("get_mean_firing_rate"), &DistributedBlanket::get_mean_firing_rate);
    ClassDB::bind_method(D_METHOD("get_active_neuron_count"), &DistributedBlanket::get_active_neuron_count);
    ClassDB::bind_method(D_METHOD("get_neuron_count"), &DistributedBlanket::get_neuron_count);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "total_vfe", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_total_vfe");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mean_firing_rate", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_mean_firing_rate");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_neuron_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_active_neuron_count");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "global_efficiency", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_global_efficiency");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "total_landauer_cost", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_total_landauer_cost");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "total_negentropy_harvested", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_total_negentropy_harvested");

}
