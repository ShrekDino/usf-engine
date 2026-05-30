#include "global_workspace.h"
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"
#include <cmath>

GlobalWorkspace::GlobalWorkspace() {
    for (int i = 0; i < 100; i++) {
        phi_star_history[i] = 0.0;
    }
}

void GlobalWorkspace::set_graph(Ref<ConnectomeGraph> p_graph) {
    graph = p_graph;
}

void GlobalWorkspace::set_blanket(Ref<DistributedBlanket> p_blanket) {
    blanket = p_blanket;
}

void GlobalWorkspace::initialize() {
    if (graph.is_null() || blanket.is_null()) {
        print_line("[Workspace] ERROR: graph and blanket required");
        return;
    }

    rich_club_indices.clear();
    rich_club_degrees.clear();
    detect_rich_club();

    phi_star = 0.0;
    ignition_level = 0.0;
    ignited = false;
    n_ignited_neurons = 0;
    phi_field = 0.0;
    phi_synchrony = 0.0;
    phi_history_idx = 0;

    print_line("[Workspace] Initialized: " + itos(n_rich_club) + " rich-club hubs (threshold=" + rtos(rich_club_threshold) + ")");
}

void GlobalWorkspace::detect_rich_club() {
    if (graph.is_null()) return;

    int n = graph->get_vertex_count();
    Vector<double> degrees;
    degrees.resize(n);

    double mean_deg = 0.0;
    double sum_sq = 0.0;

    for (int i = 0; i < n; i++) {
        double deg = (double)(graph->get_outgoing_count(i) + graph->get_incoming_count(i));
        degrees.write[i] = deg;
        mean_deg += deg;
    }
    mean_deg /= (double)n;

    for (int i = 0; i < n; i++) {
        double diff = degrees[i] - mean_deg;
        sum_sq += diff * diff;
    }
    double std_dev = std::sqrt(sum_sq / (double)n);

    // Rich-club: degree > mean + 2σ (hub threshold)
    rich_club_threshold = mean_deg + 2.0 * std_dev;

    rich_club_indices.clear();
    rich_club_degrees.clear();
    for (int i = 0; i < n; i++) {
        if (degrees[i] > rich_club_threshold) {
            rich_club_indices.push_back(i);
            rich_club_degrees.push_back(degrees[i]);
        }
    }
    n_rich_club = rich_club_indices.size();
}

bool GlobalWorkspace::is_rich_club(int idx) const {
    for (int i = 0; i < n_rich_club; i++) {
        if (rich_club_indices[i] == idx) return true;
    }
    return false;
}

double GlobalWorkspace::mutual_information_between_sets(const Vector<int> &set_a, const Vector<int> &set_b) {
    if (blanket.is_null() || set_a.size() == 0 || set_b.size() == 0) return 0.0;

    // Compute mean firing rate in each set
    double mean_a = 0.0, mean_b = 0.0;
    for (int i = 0; i < set_a.size(); i++) {
        mean_a += blanket->get_firing_rate(set_a[i]);
    }
    for (int i = 0; i < set_b.size(); i++) {
        mean_b += blanket->get_firing_rate(set_b[i]);
    }
    mean_a /= (double)set_a.size();
    mean_b /= (double)set_b.size();

    // Compute joint activity (co-variance)
    double cov = 0.0;
    for (int i = 0; i < set_a.size(); i++) {
        for (int j = 0; j < set_b.size(); j++) {
            double diff_a = blanket->get_firing_rate(set_a[i]) - mean_a;
            double diff_b = blanket->get_firing_rate(set_b[j]) - mean_b;
            cov += diff_a * diff_b;
        }
    }
    cov /= (double)(set_a.size() * set_b.size());

    double var_a = 0.0, var_b = 0.0;
    for (int i = 0; i < set_a.size(); i++) {
        double d = blanket->get_firing_rate(set_a[i]) - mean_a;
        var_a += d * d;
    }
    for (int i = 0; i < set_b.size(); i++) {
        double d = blanket->get_firing_rate(set_b[i]) - mean_b;
        var_b += d * d;
    }
    var_a /= (double)set_a.size();
    var_b /= (double)set_b.size();

    if (var_a < 1e-30 || var_b < 1e-30) return 0.0;

    // Pearson correlation
    double r = cov / std::sqrt(var_a * var_b);
    if (r > 0.99) r = 0.99;
    if (r < -0.99) r = -0.99;

    // Mutual information for Gaussian: MI = -0.5 * ln(1 - r²)
    double mi = -0.5 * std::log(1.0 - r * r);
    if (mi < 0.0) mi = 0.0;
    return mi;
}

double GlobalWorkspace::compute_phi_star(int n_samples) {
    if (blanket.is_null()) return 0.0;

    int n = blanket->get_neuron_count();
    if (n < 4) return 0.0;

    // φ* is the mutual information between two random halves of the network
    // averaged over several samples
    double total_mi = 0.0;

    for (int s = 0; s < n_samples; s++) {
        // Randomly partition neurons into two sets
        Vector<int> set_a, set_b;
        for (int i = 0; i < n; i++) {
            // Deterministic selection based on pseudo-random hash
            uint32_t h = (uint32_t)(i * 2654435761 + s * 2246822519);
            if (h % 2 == 0) {
                set_a.push_back(i);
            } else {
                set_b.push_back(i);
            }
        }

        if (set_a.size() > 0 && set_b.size() > 0) {
            double mi = mutual_information_between_sets(set_a, set_b);
            total_mi += mi;
        }
    }

    double mean_mi = total_mi / (double)n_samples;

    // φ* is the integrated information: N * MI / (N-1) normalized
    // For a fully integrated system, φ* → MI
    // For a disconnected system, φ* → 0
    double phi = mean_mi * (double)(n) / (double)(n + n_samples);

    return phi;
}

double GlobalWorkspace::compute_synchrony() {
    if (blanket.is_null() || blanket->get_neuron_count() == 0) return 0.0;

    int n = blanket->get_neuron_count();
    double mean_rate = blanket->get_mean_firing_rate();
    if (mean_rate < 0.01) return 0.0;

    // Compute Kuramoto-like order parameter
    // r = |Σ exp(iθ_k)| / N where θ_k is phase from firing rate
    double sum_cos = 0.0, sum_sin = 0.0;
    for (int i = 0; i < n && i < 1000; i++) {  // Sample up to 1000 for performance
        double rate = blanket->get_firing_rate(i);
        double phase = (rate / 200.0) * 2.0 * Math_PI;
        sum_cos += std::cos(phase);
        sum_sin += std::sin(phase);
    }
    int n_sampled = (n < 1000) ? n : 1000;
    double r = std::sqrt(sum_cos * sum_cos + sum_sin * sum_sin) / (double)n_sampled;

    return r;
}

void GlobalWorkspace::compute_ignition() {
    if (blanket.is_null()) return;

    int n = blanket->get_neuron_count();
    double threshold = 10.0; // Hz — "active" threshold

    n_ignited_neurons = 0;
    double total_firing = 0.0;
    for (int i = 0; i < n; i++) {
        double rate = blanket->get_firing_rate(i);
        if (rate > threshold) {
            n_ignited_neurons++;
        }
        total_firing += rate;
    }

    ignition_level = (double)n_ignited_neurons / (double)n;
    ignited = ignition_level > ignition_threshold;

    // Rich-club vs non-rich-club firing
    if (n_rich_club > 0) {
        double rc_sum = 0.0;
        for (int i = 0; i < n_rich_club; i++) {
            rc_sum += blanket->get_firing_rate(rich_club_indices[i]);
        }
        mean_rich_club_firing = rc_sum / (double)n_rich_club;
    }

    int non_rc_count = n - n_rich_club;
    if (non_rc_count > 0) {
        double non_rc_sum = total_firing - mean_rich_club_firing * n_rich_club;
        mean_non_rich_club_firing = non_rc_sum / (double)non_rc_count;
    }

    workspace_discrepancy = mean_rich_club_firing - mean_non_rich_club_firing;
}

void GlobalWorkspace::process_step(double dt) {
    if (blanket.is_null()) return;

    // Update synchrony
    phi_synchrony = compute_synchrony();

    // Update ignition
    compute_ignition();

    // Update φ* (periodically — every 10 steps to save compute)
    static int step_counter = 0;
    step_counter++;
    if (step_counter % 10 == 0) {
        phi_star = compute_phi_star(5);
        if (phi_history_idx < 100) {
            phi_star_history[phi_history_idx] = phi_star;
            phi_history_idx++;
        }
    }

    // Dual-scalar field φ: weighted combination of synchrony and ignition
    phi_field = 0.6 * phi_synchrony + 0.3 * ignition_level + 0.1 * phi_star;
    if (phi_field > 1.0) phi_field = 1.0;
    if (phi_field < 0.0) phi_field = 0.0;
}

double GlobalWorkspace::get_phi_star_history(int i) const {
    if (i >= 0 && i < 100) return phi_star_history[i];
    return 0.0;
}

void GlobalWorkspace::reset() {
    rich_club_indices.clear();
    rich_club_degrees.clear();
    n_rich_club = 0;
    rich_club_threshold = 0.0;
    phi_star = 0.0;
    ignition_level = 0.0;
    ignited = false;
    n_ignited_neurons = 0;
    phi_field = 0.0;
    phi_synchrony = 0.0;
    phi_history_idx = 0;
    for (int i = 0; i < 100; i++) {
        phi_star_history[i] = 0.0;
    }
}

void GlobalWorkspace::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_graph", "graph"), &GlobalWorkspace::set_graph);
    ClassDB::bind_method(D_METHOD("set_blanket", "blanket"), &GlobalWorkspace::set_blanket);

    ClassDB::bind_method(D_METHOD("initialize"), &GlobalWorkspace::initialize);
    ClassDB::bind_method(D_METHOD("process_step", "dt"), &GlobalWorkspace::process_step);
    ClassDB::bind_method(D_METHOD("reset"), &GlobalWorkspace::reset);

    ClassDB::bind_method(D_METHOD("get_rich_club_count"), &GlobalWorkspace::get_rich_club_count);
    ClassDB::bind_method(D_METHOD("get_rich_club_threshold"), &GlobalWorkspace::get_rich_club_threshold);
    ClassDB::bind_method(D_METHOD("is_rich_club", "idx"), &GlobalWorkspace::is_rich_club);

    ClassDB::bind_method(D_METHOD("get_phi_star"), &GlobalWorkspace::get_phi_star);
    ClassDB::bind_method(D_METHOD("get_phi_star_history", "i"), &GlobalWorkspace::get_phi_star_history);

    ClassDB::bind_method(D_METHOD("get_ignition_level"), &GlobalWorkspace::get_ignition_level);
    ClassDB::bind_method(D_METHOD("is_ignited"), &GlobalWorkspace::is_ignited);
    ClassDB::bind_method(D_METHOD("get_ignited_count"), &GlobalWorkspace::get_ignited_count);

    ClassDB::bind_method(D_METHOD("get_phi_field"), &GlobalWorkspace::get_phi_field);
    ClassDB::bind_method(D_METHOD("get_phi_synchrony"), &GlobalWorkspace::get_phi_synchrony);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "rich_club_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_rich_club_count");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phi_star", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_phi_star");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phi_field", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_phi_field");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phi_synchrony", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_phi_synchrony");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ignition_level", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_ignition_level");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ignited", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "is_ignited");
}
