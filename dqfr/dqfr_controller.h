#include "core/object/class_db.h"
#ifndef DQFR_CONTROLLER_H
#define DQFR_CONTROLLER_H

#include "scene/main/node.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "../core/simplicial_complex.h"
#include "../core/dynamic_coherence.h"
#include "../core/regge_tensor.h"
#include "../core/usf_constants.h"
#include "../physics/pec_solver.h"
#include "../physics/forman_ricci.h"
#include "../physics/bounce_solver.h"
#include "../thermo/szilard_engine.h"
#include "../thermo/generative_model.h"
#include "../thermo/markov_blanket.h"
#include "../connectome/distributed_blanket.h"
#include "../connectome/global_workspace.h"
#include "../connectome/vitality_monitor.h"

class DQFRController : public Node {
    GDCLASS(DQFRController, Node);

protected:
    static void _bind_methods();

public:
    enum Phase {
        PHASE_DRIFT = 0,
        PHASE_SAMPLE = 1
    };

    Phase current_phase = PHASE_DRIFT;

    double drift_duration = 1.0;
    double sample_duration = 0.1;
    double ramp_duration = 0.02;
    double drift_timer = 0.0;
    double sample_timer = 0.0;
    double temporal_velocity = 1.0;
    double cycle_count = 0;
    double total_objective_time = 0.0;
    double total_subjective_time = 0.0;

    // Core physics
    Ref<USFConstants> constants;
    Ref<SimplicialComplex> complex;
    Ref<ReggeTensor> regge;
    Ref<DynamicCoherence> coherence;
    Ref<GenerativeModel> inference;
    Ref<MarkovBlanket> blanket;
    Ref<SzilardEngine> metabolism;
    Ref<FormanRicci> forman_ricci;
    Ref<class LeeWickRegulator> lee_wick;

    PECSolver *pec_solver = nullptr;
    BounceSolver *bounce_solver = nullptr;

    // Connectome stack (Week 3-6)
    Ref<DistributedBlanket> distributed_blanket;
    Ref<GlobalWorkspace> global_workspace;
    Ref<VitalityMonitor> vitality_monitor;

    // Adiabatic mollification tracking
    double chi_prev = 0.0;
    double adiabatic_penalty = 0.0;

    DQFRController();
    ~DQFRController();

    void set_constants(Ref<USFConstants> p_constants);
    void set_complex(Ref<SimplicialComplex> p_complex);

    void set_drift_duration(double d);
    double get_drift_duration() const { return drift_duration; }
    void set_sample_duration(double d);
    double get_sample_duration() const { return sample_duration; }

    double blanket_permeability(double t, double tau) const;
    double lapse_rate(double g_00) const;

    void transition_to_drift();
    void transition_to_sample();

    void _process(double delta);

    Phase get_phase() const { return current_phase; }
    double get_phase_fraction() const;
    double get_dqfr_ratio() const;
    double get_temporal_velocity() const { return temporal_velocity; }
    double get_cycle_count() const { return cycle_count; }
    double get_total_objective_time() const { return total_objective_time; }
    double get_total_subjective_time() const { return total_subjective_time; }
    double get_adiabatic_penalty() const { return adiabatic_penalty; }

    Ref<GenerativeModel> get_inference_ref() const { return inference; }
    Ref<MarkovBlanket> get_blanket_ref() const { return blanket; }
    Ref<FormanRicci> get_forman_ricci_ref() const { return forman_ricci; }
    Ref<SimplicialComplex> get_complex_ref() const { return complex; }

    void clear();
};

#endif // DQFR_CONTROLLER_H
