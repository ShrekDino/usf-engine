#include "thermo/generative_model.h"
#include "core/math/math_funcs.h"
#include <cmath>

GenerativeModel::GenerativeModel() {
    constants.instantiate();
}

void GenerativeModel::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
}

void GenerativeModel::initialize(int p_n_states, int p_n_observations) {
    n_states = p_n_states;
    n_observations = p_n_observations;

    beliefs.resize(n_states);
    prior.resize(n_states);
    double uniform_state = 1.0 / (double)n_states;
    for (int i = 0; i < n_states; i++) {
        beliefs.set(i, uniform_state);
        prior.set(i, uniform_state);
    }

    likelihood.resize(n_states);
    double uniform_obs = 1.0 / (double)n_observations;
    for (int i = 0; i < n_states; i++) {
        likelihood[i].resize(n_observations);
        for (int j = 0; j < n_observations; j++) {
            likelihood[i].set(j, uniform_obs);
        }
    }
}

void GenerativeModel::from_params(Vector<double> p_beliefs, Vector<Vector<double>> p_likelihood, Vector<double> p_prior) {
    beliefs = p_beliefs;
    likelihood = p_likelihood;
    prior = p_prior;
    n_states = beliefs.size();
    if (likelihood.size() > 0) {
        n_observations = likelihood[0].size();
    } else {
        n_observations = 1;
    }
}

double GenerativeModel::variational_free_energy(Vector<double> observation) const {
    double complexity = 0.0;
    for (int i = 0; i < n_states; i++) {
        double q = beliefs[i];
        double p = prior[i];
        if (q > 0.0 && p > 0.0) {
            complexity += q * std::log(q / p);
        }
    }

    double inaccuracy = 0.0;
    for (int i = 0; i < n_states; i++) {
        double q = beliefs[i];
        for (int j = 0; j < observation.size(); j++) {
            int obs_idx = j % n_observations;
            double p_oj_si = likelihood[i][obs_idx];
            double o_j = observation[j];
            if (q > 0.0 && p_oj_si > 0.0 && o_j > 0.0) {
                inaccuracy += q * o_j * std::log(p_oj_si);
            }
        }
    }

    if (inaccuracy < 0.0) {
        inaccuracy = -inaccuracy;
    }

    return complexity + inaccuracy;
}

void GenerativeModel::update_beliefs(Vector<double> observation, double learning_rate) {
    Vector<double> posterior;
    posterior.resize(n_states);
    for (int i = 0; i < n_states; i++) {
        posterior.set(i, prior[i]);
    }

    for (int i = 0; i < n_states; i++) {
        for (int j = 0; j < observation.size(); j++) {
            int obs_idx = j % n_observations;
            double p_oj_si = likelihood[i][obs_idx];
            double o_j = observation[j];
            if (o_j > 0.0) {
                posterior.set(i, posterior[i] * std::pow(p_oj_si, o_j));
            }
        }
    }

    double total = 0.0;
    for (int i = 0; i < n_states; i++) {
        total += posterior[i];
    }
    if (total > 0.0) {
        for (int i = 0; i < n_states; i++) {
            posterior.set(i, posterior[i] / total);
        }
    }

    for (int i = 0; i < n_states; i++) {
        double updated = (1.0 - learning_rate) * beliefs[i] + learning_rate * posterior[i];
        beliefs.set(i, updated);
    }
}

double GenerativeModel::predictive_accuracy() const {
    double sum_sq = 0.0;
    for (int i = 0; i < n_states; i++) {
        sum_sq += beliefs[i] * beliefs[i];
    }
    return sum_sq / (double)n_states;
}

double GenerativeModel::entropy() const {
    double h = 0.0;
    for (int i = 0; i < n_states; i++) {
        double b = beliefs[i];
        if (b > 0.0) {
            h -= b * std::log(b);
        }
    }
    return h;
}

double GenerativeModel::metabolic_cost() const {
    double temp = 300.0;
    return constants->k_b * temp * entropy();
}

double GenerativeModel::get_belief(int i) const {
    if (i >= 0 && i < beliefs.size()) {
        return beliefs[i];
    }
    return 0.0;
}

void GenerativeModel::set_belief(int i, double val) {
    if (i >= 0 && i < beliefs.size()) {
        beliefs.set(i, val);
    }
}

void GenerativeModel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &GenerativeModel::set_constants);
    ClassDB::bind_method(D_METHOD("get_constants"), &GenerativeModel::get_constants);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "constants", PROPERTY_HINT_RESOURCE_TYPE, "USFConstants"), "set_constants", "get_constants");

    ClassDB::bind_method(D_METHOD("initialize", "n_states", "n_observations"), &GenerativeModel::initialize);
    ClassDB::bind_method(D_METHOD("variational_free_energy", "observation"), &GenerativeModel::variational_free_energy);
    ClassDB::bind_method(D_METHOD("update_beliefs", "observation", "learning_rate"), &GenerativeModel::update_beliefs);

    ClassDB::bind_method(D_METHOD("predictive_accuracy"), &GenerativeModel::predictive_accuracy);
    ClassDB::bind_method(D_METHOD("entropy"), &GenerativeModel::entropy);
    ClassDB::bind_method(D_METHOD("metabolic_cost"), &GenerativeModel::metabolic_cost);

    ClassDB::bind_method(D_METHOD("get_n_states"), &GenerativeModel::get_n_states);
    ClassDB::bind_method(D_METHOD("get_n_observations"), &GenerativeModel::get_n_observations);
    ClassDB::bind_method(D_METHOD("get_belief", "i"), &GenerativeModel::get_belief);
    ClassDB::bind_method(D_METHOD("set_belief", "i", "val"), &GenerativeModel::set_belief);
}
