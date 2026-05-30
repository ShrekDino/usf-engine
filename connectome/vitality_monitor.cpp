#include "vitality_monitor.h"
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"
#include <cmath>

VitalityMonitor::VitalityMonitor() {
    for (int i = 0; i < 100; i++) {
        v_network_history[i] = 0.0;
    }
}

void VitalityMonitor::set_graph(Ref<ConnectomeGraph> p_graph) { graph = p_graph; }
void VitalityMonitor::set_blanket(Ref<DistributedBlanket> p_blanket) { blanket = p_blanket; }
void VitalityMonitor::set_workspace(Ref<GlobalWorkspace> p_workspace) { workspace = p_workspace; }

void VitalityMonitor::initialize() {
    if (blanket.is_null()) {
        print_line("[Vitality] ERROR: blanket required");
        return;
    }

    int n = blanket->get_neuron_count();
    neuron_efficiency.resize(n);
    senescence_counter.resize(n);
    senescent.resize(n);

    for (int i = 0; i < n; i++) {
        neuron_efficiency.write[i] = 0.0;
        senescence_counter.write[i] = 0;
        senescent.write[i] = false;
    }

    senescent_count = 0;
    v_network = 0.0;
    mutual_info_sum = 0.0;
    vfe_history.clear();
    vfe_variance = 0.0;
    curiosity_drive = 0.0;
    exploring = false;
    curiosity_cooldown = 0;
    halt_risk_neurons = 0;
    halt_prevention_triggers = 0;
    vn_history_idx = 0;

    print_line("[Vitality] Initialized: " + itos(n) + " neurons monitored");
}

double VitalityMonitor::get_neuron_efficiency(int idx) const {
    if (idx >= 0 && idx < neuron_efficiency.size()) return neuron_efficiency[idx];
    return 0.0;
}

void VitalityMonitor::compute_senescence() {
    if (blanket.is_null()) return;
    int n = blanket->get_neuron_count();
    senescent_count = 0;

    // Note: blanket's states array is public but not accessible via getter.
    // We approximate efficiency from global metrics.
    double global_eff = blanket->get_global_efficiency();

    for (int i = 0; i < n; i++) {
        // Approximate per-neuron efficiency from firing rate vs negentropy
        double rate = blanket->get_firing_rate(i);
        double neg = blanket->get_negentropy(i);
        double eff = (rate > 0.5 && neg > 0.0) ? (neg / (neg + 1.0)) : 0.0;
        neuron_efficiency.write[i] = eff;

        if (eff < senescence_threshold) {
            senescence_counter.write[i]++;
            if (senescence_counter[i] > senescence_max_steps) {
                senescent.write[i] = true;
                senescent_count++;
            }
        } else {
            senescence_counter.write[i] = 0;
            senescent.write[i] = false;
        }
    }
}

void VitalityMonitor::compute_v_network(double dt) {
    if (blanket.is_null() || graph.is_null()) return;

    int n = blanket->get_neuron_count();
    double total_v = 0.0;
    double total_mi = 0.0;
    double k_B = 1.380649e-23;
    int pair_count = 0;

    // V = Σ(k_B · ε_i · H_i + Σλ_ij · I(μ_i; μ_j))
    for (int i = 0; i < n; i++) {
        double eff = neuron_efficiency[i];
        double firing = blanket->get_firing_rate(i);
        double neg = blanket->get_negentropy(i);
        double H_env = (neg > 0.0) ? std::log(1.0 + neg) : 0.0;

        // Thermodynamic term: k_B · ε_i · H_i
        total_v += k_B * eff * H_env;

        // Mutual information between connected neurons (sample for performance)
        int n_out = graph->get_outgoing_count(i);
        for (int j = 0; j < n_out && j < 5; j++) {
            int ei = graph->get_outgoing_edge(i, j);
            int post = graph->get_edge_post(ei);
            if (post >= 0 && post < n) {
                double rate_i = blanket->get_firing_rate(i);
                double rate_j = blanket->get_firing_rate(post);
                double coupling = (double)graph->get_edge_synapse_count(ei);
                double mi = rate_i * rate_j * coupling * 1e-6;
                total_mi += mi;
                total_v += mi;
                pair_count++;
            }
        }
    }

    v_network = total_v;
    mutual_info_sum = total_mi;

    if (vn_history_idx < 100) {
        v_network_history[vn_history_idx] = v_network;
        vn_history_idx++;
    }
}

void VitalityMonitor::compute_curiosity(double dt) {
    if (blanket.is_null()) return;

    double vfe = blanket->get_total_vfe();
    vfe_history.push_back(vfe);
    if (vfe_history.size() > 50) {
        vfe_history.remove_at(0);
    }

    // Compute VFE variance
    if (vfe_history.size() >= 10) {
        double mean = 0.0;
        for (int i = 0; i < vfe_history.size(); i++) mean += vfe_history[i];
        mean /= (double)vfe_history.size();

        double var = 0.0;
        for (int i = 0; i < vfe_history.size(); i++) {
            double d = vfe_history[i] - mean;
            var += d * d;
        }
        vfe_variance = var / (double)vfe_history.size();
    }

    // Curiosity: when VFE stabilizes too perfectly, force exploration
    if (curiosity_cooldown > 0) {
        curiosity_cooldown--;
        exploring = false;
        curiosity_drive = 0.0;
        return;
    }

    if (vfe_variance < curiosity_threshold && vfe_history.size() >= 20) {
        exploring = true;
        curiosity_drive = (curiosity_threshold - vfe_variance) / curiosity_threshold;
        if (curiosity_drive > 1.0) curiosity_drive = 1.0;

        // Inject noise into membrane potentials to force exploration
        double noise_amplitude = curiosity_drive * 10.0;
        for (int i = 0; i < blanket->get_neuron_count(); i++) {
            if (blanket->get_firing_rate(i) < 1.0) {
                double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 * noise_amplitude;
                // We can't directly modify membrane_potential from here,
                // but we can flag that exploration was triggered
            }
        }
        curiosity_cooldown = 100;
    } else {
        exploring = false;
        curiosity_drive = 0.0;
    }
}

void VitalityMonitor::compute_halt_prevention() {
    if (blanket.is_null()) return;

    int n = blanket->get_neuron_count();
    halt_risk_neurons = 0;

    for (int i = 0; i < n; i++) {
        double eff = neuron_efficiency[i];
        double neg = blanket->get_negentropy(i);
        double rate = blanket->get_firing_rate(i);

        // HALT risk: efficiency near 0 AND negentropy depleted AND no firing
        if (eff < 0.01 && neg < 1.0 && rate < 0.5) {
            halt_risk_neurons++;
        }
    }
}

void VitalityMonitor::process_step(double dt) {
    if (blanket.is_null()) return;

    compute_senescence();
    compute_v_network(dt);
    compute_curiosity(dt);
    compute_halt_prevention();
}

void VitalityMonitor::reset() {
    neuron_efficiency.clear();
    senescence_counter.clear();
    senescent.clear();
    senescent_count = 0;
    v_network = 0.0;
    mutual_info_sum = 0.0;
    vfe_history.clear();
    vfe_variance = 0.0;
    curiosity_drive = 0.0;
    exploring = false;
    curiosity_cooldown = 0;
    halt_risk_neurons = 0;
    halt_prevention_triggers = 0;
    vn_history_idx = 0;
}

void VitalityMonitor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_graph", "graph"), &VitalityMonitor::set_graph);
    ClassDB::bind_method(D_METHOD("set_blanket", "blanket"), &VitalityMonitor::set_blanket);
    ClassDB::bind_method(D_METHOD("set_workspace", "workspace"), &VitalityMonitor::set_workspace);

    ClassDB::bind_method(D_METHOD("initialize"), &VitalityMonitor::initialize);
    ClassDB::bind_method(D_METHOD("process_step", "dt"), &VitalityMonitor::process_step);
    ClassDB::bind_method(D_METHOD("reset"), &VitalityMonitor::reset);

    ClassDB::bind_method(D_METHOD("get_senescent_count"), &VitalityMonitor::get_senescent_count);
    ClassDB::bind_method(D_METHOD("get_halt_risk_neurons"), &VitalityMonitor::get_halt_risk_neurons);
    ClassDB::bind_method(D_METHOD("get_neuron_efficiency", "idx"), &VitalityMonitor::get_neuron_efficiency);

    ClassDB::bind_method(D_METHOD("get_v_network"), &VitalityMonitor::get_v_network);
    ClassDB::bind_method(D_METHOD("get_mutual_info_sum"), &VitalityMonitor::get_mutual_info_sum);

    ClassDB::bind_method(D_METHOD("get_curiosity_drive"), &VitalityMonitor::get_curiosity_drive);
    ClassDB::bind_method(D_METHOD("is_exploring"), &VitalityMonitor::is_exploring);

    ClassDB::bind_method(D_METHOD("get_halt_prevention_triggers"), &VitalityMonitor::get_halt_prevention_triggers);
    ClassDB::bind_method(D_METHOD("get_vfe_variance"), &VitalityMonitor::get_vfe_variance);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "senescent_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_senescent_count");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "v_network", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_v_network");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "curiosity_drive", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_curiosity_drive");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "exploring", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "is_exploring");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vfe_variance", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_vfe_variance");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "halt_risk_neurons", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_halt_risk_neurons");
}
