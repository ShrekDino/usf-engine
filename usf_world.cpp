#include "usf_world.h"
#include "scene/main/scene_tree.h"
#include "core/math/math_funcs.h"

USFWorld::USFWorld() {
    constants.instantiate();
    regge.instantiate();
    lee_wick.instantiate();
    coherence.instantiate();
    forman_ricci.instantiate();
    inference.instantiate();
    blanket.instantiate();
    metabolism.instantiate();
}

USFWorld::~USFWorld() {
}

void USFWorld::_ready() {
    if (!initialized) {
        initialize_world();
    }
}

void USFWorld::initialize_world() {
    constants->recompute();

    complex.instantiate();
    complex->from_hypercubic_lattice(lattice_n, 1.0, constants);

    regge->compute(complex);

    lee_wick->set_constants(constants);

    coherence->set_base_radius(constants->planck_length * 1e6);

    forman_ricci->set_constants(constants);
    forman_ricci->set_damping_coefficient(1.0);

    inference->set_constants(constants);
    inference->initialize(4, 3);

    blanket->initialize(4, 3, 2, 4);

    metabolism->set_constants(constants);
    metabolism->initialize(1000.0, 300.0);

    if (!pec_solver) {
        pec_solver = memnew(PECSolver);
        add_child(pec_solver);
        pec_solver->set_constants(constants);
        pec_solver->set_complex(complex);
    }

    if (!bounce_solver) {
        bounce_solver = memnew(BounceSolver);
        add_child(bounce_solver);
        bounce_solver->set_constants(constants);
        bounce_solver->initialize(1.0, 1e-10, 1e-12, 0.0);
    }

    if (!dqfr) {
        dqfr = memnew(DQFRController);
        add_child(dqfr);
        dqfr->set_constants(constants);
        dqfr->set_complex(complex);
        dqfr->set_drift_duration(1.0);
        dqfr->set_sample_duration(0.1);
        dqfr->pec_solver = pec_solver;
        dqfr->bounce_solver = bounce_solver;
        dqfr->regge = regge;
        dqfr->coherence = coherence;
        dqfr->forman_ricci = forman_ricci;
        dqfr->inference = inference;
        dqfr->blanket = blanket;
        dqfr->metabolism = metabolism;
        dqfr->lee_wick = lee_wick;
        dqfr->complex = complex;
    }

    initialized = true;
}

void USFWorld::clear_world() {
    if (dqfr) {
        dqfr->clear();
    }
    if (pec_solver) {
        pec_solver->reset();
    }
    variational_free_energy = 0.0;
    dqfr_ratio = 1.0;
    omega_coherence = 0.0;
    temporal_velocity = 1.0;
    initialized = false;
}

void USFWorld::_process(double delta) {
    if (!initialized) {
        return;
    }

    if (coherence.is_valid()) {
        omega_coherence = coherence->get_omega_coherence();
    }

    dqfr_ratio = dqfr ? dqfr->get_dqfr_ratio() : 1.0;

    temporal_velocity = dqfr ? dqfr->get_temporal_velocity() : 1.0;

    if (bounce_solver) {
        bounce_a_min = bounce_solver->get_a_min();
        bounce_detected = bounce_solver->get_bounce_detected();
    }

    if (inference.is_valid() && blanket.is_valid()) {
        Vector<double> blank_obs;
        if (blanket->sensory_states.size() > 0) {
            blank_obs = blanket->sensory_states;
        } else {
            blank_obs.push_back(0.5);
            blank_obs.push_back(0.3);
            blank_obs.push_back(0.2);
        }
        variational_free_energy = inference->variational_free_energy(blank_obs);
    }

    if (metabolism.is_valid()) {
        metabolic_efficiency = metabolism->get_efficiency();
    }

    update_gdscript_properties();
}

void USFWorld::update_gdscript_properties() {
}

void USFWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize_world"), &USFWorld::initialize_world);
    ClassDB::bind_method(D_METHOD("clear_world"), &USFWorld::clear_world);

    ClassDB::bind_method(D_METHOD("set_lattice_n", "n"), &USFWorld::set_lattice_n);
    ClassDB::bind_method(D_METHOD("get_lattice_n"), &USFWorld::get_lattice_n);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "lattice_n", PROPERTY_HINT_RANGE, "2,10,1"), "set_lattice_n", "get_lattice_n");

    ClassDB::bind_method(D_METHOD("get_variational_free_energy"), &USFWorld::get_variational_free_energy);
    ClassDB::bind_method(D_METHOD("get_dqfr_ratio"), &USFWorld::get_dqfr_ratio);
    ClassDB::bind_method(D_METHOD("get_omega_coherence"), &USFWorld::get_omega_coherence);
    ClassDB::bind_method(D_METHOD("get_temporal_velocity"), &USFWorld::get_temporal_velocity);
    ClassDB::bind_method(D_METHOD("get_bounce_a_min"), &USFWorld::get_bounce_a_min);
    ClassDB::bind_method(D_METHOD("get_metabolic_efficiency"), &USFWorld::get_metabolic_efficiency);
    ClassDB::bind_method(D_METHOD("get_bounce_detected"), &USFWorld::get_bounce_detected);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "variational_free_energy", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_variational_free_energy");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dqfr_ratio", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_dqfr_ratio");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "omega_coherence", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_omega_coherence");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temporal_velocity", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_temporal_velocity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bounce_a_min", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_bounce_a_min");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "metabolic_efficiency", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_metabolic_efficiency");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bounce_detected", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_bounce_detected");
}
