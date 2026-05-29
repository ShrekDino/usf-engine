#ifndef USF_WORLD_H
#define USF_WORLD_H

#include "scene/3d/node_3d.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/simplicial_complex.h"
#include "core/dynamic_coherence.h"
#include "core/regge_tensor.h"
#include "core/lee_wick.h"
#include "core/usf_constants.h"
#include "physics/pec_solver.h"
#include "physics/bounce_solver.h"
#include "physics/forman_ricci.h"
#include "physics/soliton.h"
#include "physics/membrane.h"
#include "thermo/szilard_engine.h"
#include "thermo/generative_model.h"
#include "thermo/markov_blanket.h"
#include "dqfr/dqfr_controller.h"

class USFWorld : public Node3D {
    GDCLASS(USFWorld, Node3D);

protected:
    static void _bind_methods();

public:
    Ref<USFConstants> constants;
    Ref<SimplicialComplex> complex;
    Ref<ReggeTensor> regge;
    Ref<LeeWickRegulator> lee_wick;
    Ref<DynamicCoherence> coherence;
    Ref<FormanRicci> forman_ricci;
    Ref<GenerativeModel> inference;
    Ref<MarkovBlanket> blanket;
    Ref<SzilardEngine> metabolism;

    DQFRController *dqfr = nullptr;
    PECSolver *pec_solver = nullptr;
    BounceSolver *bounce_solver = nullptr;

    bool initialized = false;
    int lattice_n = 4;

    USFWorld();
    ~USFWorld();

    void initialize_world();
    void clear_world();

    void _ready() override;
    void _process(double delta) override;

    double get_variational_free_energy() const { return variational_free_energy; }
    double get_dqfr_ratio() const { return dqfr_ratio; }
    double get_omega_coherence() const { return omega_coherence; }
    double get_temporal_velocity() const { return temporal_velocity; }
    double get_bounce_a_min() const { return bounce_a_min; }
    double get_metabolic_efficiency() const { return metabolic_efficiency; }
    bool get_bounce_detected() const { return bounce_detected; }

    void set_lattice_n(int n) { lattice_n = n > 1 ? n : 4; }
    int get_lattice_n() const { return lattice_n; }

private:
    double variational_free_energy = 0.0;
    double dqfr_ratio = 1.0;
    double omega_coherence = 0.0;
    double temporal_velocity = 1.0;
    double bounce_a_min = 0.0;
    double metabolic_efficiency = 0.0;
    bool bounce_detected = false;

    void update_gdscript_properties();
};

#endif // USF_WORLD_H
