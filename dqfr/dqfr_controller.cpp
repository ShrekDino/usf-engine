#include "core/math/math_defs.h"
#include "../core/lee_wick.h"
#include "core/object/class_db.h"
#include "dqfr_controller.h"
#include "core/math/math_funcs.h"
#include <cmath>

DQFRController::DQFRController() {
    regge.instantiate();
    coherence.instantiate();
    inference.instantiate();
    blanket.instantiate();
    metabolism.instantiate();
    forman_ricci.instantiate();
    lee_wick.instantiate();
}

DQFRController::~DQFRController() {
}

void DQFRController::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
    if (forman_ricci.is_valid()) {
        forman_ricci->set_constants(constants);
    }
    if (metabolism.is_valid()) {
        metabolism->set_constants(constants);
    }
    if (inference.is_valid()) {
        inference->set_constants(constants);
    }
    if (lee_wick.is_valid()) {
        lee_wick->set_constants(constants);
    }
    if (coherence.is_valid()) {
        coherence->set_base_radius(constants->planck_length * 1e6);
    }
}

void DQFRController::set_complex(Ref<SimplicialComplex> p_complex) {
    complex = p_complex;
}

void DQFRController::set_drift_duration(double d) {
    drift_duration = d > 0.0 ? d : 1.0;
}

void DQFRController::set_sample_duration(double d) {
    sample_duration = d > 0.0 ? d : 0.1;
}

double DQFRController::blanket_permeability(double t, double tau) const {
    if (t <= 0.0) return 0.0;
    if (t >= tau) return 1.0;
    return 0.5 * (1.0 - std::cos(Math::PI * t / tau));
}

double DQFRController::lapse_rate(double g_00) const {
    if (g_00 <= 0.0) return 1.0;
    return std::sqrt(g_00);
}

void DQFRController::transition_to_drift() {
    current_phase = PHASE_DRIFT;
    drift_timer = 0.0;

    if (blanket.is_valid()) {
        blanket->seal();
    }

    temporal_velocity = 1.0 / (1.0 + drift_duration / (sample_duration + 1e-30));
}

void DQFRController::transition_to_sample() {
    current_phase = PHASE_SAMPLE;
    sample_timer = 0.0;
    cycle_count += 1.0;
}

void DQFRController::_process(double delta) {
    if (constants.is_null() || complex.is_null()) {
        return;
    }

    double lapse = 1.0;
    if (pec_solver && pec_solver->tetrads.size() > 0) {
        double g_00 = pec_solver->tetrads[0]->metric_component(0, 0);
        lapse = lapse_rate(g_00);
    }

    double scaled_delta = delta * lapse;

    switch (current_phase) {
        case PHASE_DRIFT: {
            drift_timer += scaled_delta;

            if (coherence.is_valid()) {
                coherence->omega_coherence = coherence->omega_0;
            }

            if (drift_timer >= drift_duration) {
                transition_to_sample();
            }
            break;
        }

        case PHASE_SAMPLE: {
            sample_timer += scaled_delta;
            double chi = blanket_permeability(sample_timer, ramp_duration);
            double effective_dt = scaled_delta * chi;

            if (blanket.is_valid() && chi > 0.01) {
                blanket->unshield(inference->beliefs);
            }

            if (chi > 0.01) {
                if (pec_solver) {
                    pec_solver->solve_step();
                }

                if (forman_ricci.is_valid() && complex.is_valid()) {
                    forman_ricci->damp_metric(complex, effective_dt);
                }

                if (coherence.is_valid() && regge.is_valid()) {
                    double avg_curv = regge->average_curvature();
                    if (avg_curv > 0.0) {
                        double integrated = avg_curv * effective_dt;
                        coherence->recompute_from_ricci(integrated);
                    }
                }

                if (bounce_solver) {
                    bounce_solver->step(effective_dt);
                }
            }

            if (metabolism.is_valid()) {
                metabolism->step(effective_dt);
            }

            if (inference.is_valid() && blanket.is_valid()) {
                Vector<double> sensory_copy = blanket->sensory_states;
                if (sensory_copy.size() > 0) {
                    inference->update_beliefs(sensory_copy, 0.1 * chi);
                }
            }

            if (lee_wick.is_valid() && complex.is_valid()) {
                for (int ei = 0; ei < complex->edge_count(); ei++) {
                    double clamped = lee_wick->clamp_edge_length(complex->edge_lengths[ei]);
                    complex->edge_lengths.set(ei, clamped);
                }
            }

            if (sample_timer >= sample_duration) {
                transition_to_drift();
            }
            break;
        }
    }
}

double DQFRController::get_phase_fraction() const {
    switch (current_phase) {
        case PHASE_DRIFT:
            return drift_duration > 0.0 ? drift_timer / drift_duration : 0.0;
        case PHASE_SAMPLE:
            return sample_duration > 0.0 ? sample_timer / sample_duration : 0.0;
    }
    return 0.0;
}

double DQFRController::get_dqfr_ratio() const {
    double total = drift_duration + sample_duration;
    if (total <= 0.0) return 1.0;
    return drift_duration / total;
}

void DQFRController::clear() {
    current_phase = PHASE_DRIFT;
    drift_timer = 0.0;
    sample_timer = 0.0;
    temporal_velocity = 1.0;
    cycle_count = 0.0;

    if (blanket.is_valid()) {
        blanket->seal();
    }
}

void DQFRController::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &DQFRController::set_constants);
    ClassDB::bind_method(D_METHOD("set_complex", "complex"), &DQFRController::set_complex);

    ClassDB::bind_method(D_METHOD("set_drift_duration", "d"), &DQFRController::set_drift_duration);
    ClassDB::bind_method(D_METHOD("get_drift_duration"), &DQFRController::get_drift_duration);
    ClassDB::bind_method(D_METHOD("set_sample_duration", "d"), &DQFRController::set_sample_duration);
    ClassDB::bind_method(D_METHOD("get_sample_duration"), &DQFRController::get_sample_duration);

    ClassDB::bind_method(D_METHOD("blanket_permeability", "t", "tau"), &DQFRController::blanket_permeability);
    ClassDB::bind_method(D_METHOD("lapse_rate", "g_00"), &DQFRController::lapse_rate);

    ClassDB::bind_method(D_METHOD("get_phase"), &DQFRController::get_phase);
    ClassDB::bind_method(D_METHOD("get_phase_fraction"), &DQFRController::get_phase_fraction);
    ClassDB::bind_method(D_METHOD("get_dqfr_ratio"), &DQFRController::get_dqfr_ratio);
    ClassDB::bind_method(D_METHOD("get_temporal_velocity"), &DQFRController::get_temporal_velocity);
    ClassDB::bind_method(D_METHOD("get_cycle_count"), &DQFRController::get_cycle_count);

    ClassDB::bind_method(D_METHOD("get_distributed_blanket_ref"), &DQFRController::get_distributed_blanket_ref);
    ClassDB::bind_method(D_METHOD("set_distributed_blanket_ref", "blanket"), &DQFRController::set_distributed_blanket_ref);
    ClassDB::bind_method(D_METHOD("get_global_workspace_ref"), &DQFRController::get_global_workspace_ref);
    ClassDB::bind_method(D_METHOD("set_global_workspace_ref", "workspace"), &DQFRController::set_global_workspace_ref);
    ClassDB::bind_method(D_METHOD("get_vitality_monitor_ref"), &DQFRController::get_vitality_monitor_ref);
    ClassDB::bind_method(D_METHOD("set_vitality_monitor_ref", "monitor"), &DQFRController::set_vitality_monitor_ref);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "distributed_blanket", PROPERTY_HINT_RESOURCE_TYPE, "DistributedBlanket"), "set_distributed_blanket_ref", "get_distributed_blanket_ref");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "global_workspace", PROPERTY_HINT_RESOURCE_TYPE, "GlobalWorkspace"), "set_global_workspace_ref", "get_global_workspace_ref");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "vitality_monitor", PROPERTY_HINT_RESOURCE_TYPE, "VitalityMonitor"), "set_vitality_monitor_ref", "get_vitality_monitor_ref");

    ClassDB::bind_method(D_METHOD("clear"), &DQFRController::clear);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drift_duration", PROPERTY_HINT_RANGE, "0.001,100.0,0.001"), "set_drift_duration", "get_drift_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sample_duration", PROPERTY_HINT_RANGE, "0.001,100.0,0.001"), "set_sample_duration", "get_sample_duration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dqfr_ratio", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_dqfr_ratio");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temporal_velocity", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_temporal_velocity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phase_fraction", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_phase_fraction");

    BIND_ENUM_CONSTANT(PHASE_DRIFT);
    BIND_ENUM_CONSTANT(PHASE_SAMPLE);
}
VARIANT_ENUM_CAST(DQFRController::Phase);
