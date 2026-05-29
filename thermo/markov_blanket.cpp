#include "core/math/math_defs.h"
#include "markov_blanket.h"
#include "core/math/math_funcs.h"
#include <cmath>

MarkovBlanket::MarkovBlanket() {}

void MarkovBlanket::initialize(int n_internal, int n_sensory, int n_active, int n_external) {
    internal_indices.resize(n_internal);
    for (int i = 0; i < n_internal; i++) {
        internal_indices.set(i, i);
    }

    sensory_indices.resize(n_sensory);
    for (int i = 0; i < n_sensory; i++) {
        sensory_indices.set(i, n_internal + i);
    }

    active_indices.resize(n_active);
    for (int i = 0; i < n_active; i++) {
        active_indices.set(i, n_internal + n_sensory + i);
    }

    external_indices.resize(n_external);
    for (int i = 0; i < n_external; i++) {
        external_indices.set(i, n_internal + n_sensory + n_active + i);
    }

    internal_states.resize(n_internal);
    sensory_states.resize(n_sensory);
    active_states.resize(n_active);
    external_states.resize(n_external);

    for (int i = 0; i < n_internal; i++) internal_states.set(i, 0.0);
    for (int i = 0; i < n_sensory; i++) sensory_states.set(i, 0.0);
    for (int i = 0; i < n_active; i++) active_states.set(i, 0.0);
    for (int i = 0; i < n_external; i++) external_states.set(i, 0.0);
}

bool MarkovBlanket::is_permeable() const {
    for (int i = 0; i < sensory_states.size(); i++) {
        if (std::abs(sensory_states[i]) > 1e-10) return true;
    }
    for (int i = 0; i < active_states.size(); i++) {
        if (std::abs(active_states[i]) > 1e-10) return true;
    }
    return false;
}

double MarkovBlanket::mutual_information() const {
    double i_sensory = 0.0;
    for (int i = 0; i < sensory_states.size(); i++) {
        i_sensory += sensory_states[i] * sensory_states[i];
    }
    double i_active = 0.0;
    for (int i = 0; i < active_states.size(); i++) {
        i_active += active_states[i] * active_states[i];
    }
    double i_external = 0.0;
    for (int i = 0; i < external_states.size(); i++) {
        i_external += external_states[i] * external_states[i];
    }

    if (i_external > 0.0) {
        return (i_sensory + i_active) / i_external;
    }
    return 0.0;
}

void MarkovBlanket::sense(Vector<double> external_signals) {
    int n = sensory_states.size();
    if (external_signals.size() < n) {
        n = external_signals.size();
    }
    for (int i = 0; i < n; i++) {
        sensory_states.set(i, external_signals[i]);
    }
}

Vector<double> MarkovBlanket::act() {
    double coupling = 0.0;
    for (int i = 0; i < internal_states.size(); i++) {
        coupling += internal_states[i];
    }
    coupling /= (double)internal_states.size();

    Vector<double> output;
    output.resize(active_states.size());
    for (int i = 0; i < active_states.size(); i++) {
        output.set(i, active_states[i] * (1.0 + coupling * 0.1));
    }
    return output;
}

void MarkovBlanket::update_internal(double learning_rate) {
    double sensory_avg = 0.0;
    for (int i = 0; i < sensory_states.size(); i++) {
        sensory_avg += sensory_states[i];
    }
    sensory_avg /= (double)sensory_states.size();

    for (int i = 0; i < internal_states.size(); i++) {
        double val = internal_states[i] + learning_rate * (sensory_avg - internal_states[i]);
        internal_states.set(i, val);
    }
}

double MarkovBlanket::entropy() const {
    double variance = 0.0;
    for (int i = 0; i < internal_states.size(); i++) {
        variance += internal_states[i] * internal_states[i];
    }
    variance /= (double)internal_states.size();

    if (variance > 0.0) {
        return 0.5 * std::log(2.0 * Math::PI * Math::E * variance);
    }
    return 0.0;
}

double MarkovBlanket::vitality() const {
    double m_info = mutual_information();
    double perm = is_permeable() ? 1.0 : 0.0;
    return m_info * perm;
}

void MarkovBlanket::seal() {
    for (int i = 0; i < sensory_states.size(); i++) {
        sensory_states.set(i, 0.0);
    }
    for (int i = 0; i < active_states.size(); i++) {
        active_states.set(i, 0.0);
    }
}

void MarkovBlanket::unshield(Vector<double> external_signals) {
    sense(external_signals);
}

void MarkovBlanket::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "n_internal", "n_sensory", "n_active", "n_external"), &MarkovBlanket::initialize);

    ClassDB::bind_method(D_METHOD("is_permeable"), &MarkovBlanket::is_permeable);
    ClassDB::bind_method(D_METHOD("mutual_information"), &MarkovBlanket::mutual_information);
    ClassDB::bind_method(D_METHOD("sense", "external_signals"), &MarkovBlanket::sense);
    ClassDB::bind_method(D_METHOD("act"), &MarkovBlanket::act);
    ClassDB::bind_method(D_METHOD("update_internal", "learning_rate"), &MarkovBlanket::update_internal);

    ClassDB::bind_method(D_METHOD("entropy"), &MarkovBlanket::entropy);
    ClassDB::bind_method(D_METHOD("vitality"), &MarkovBlanket::vitality);

    ClassDB::bind_method(D_METHOD("seal"), &MarkovBlanket::seal);
    ClassDB::bind_method(D_METHOD("unshield", "external_signals"), &MarkovBlanket::unshield);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vitality", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "vitality");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "entropy", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "entropy");
}
