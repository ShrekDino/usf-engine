#include "register_types.h"
#include "modules/register_module_types.h"
#include "core/object/class_db.h"
#include "core/usf_constants.h"
#include "core/simplicial_complex.h"
#include "core/dynamic_coherence.h"
#include "core/lee_wick.h"
#include "core/regge_tensor.h"
#include "physics/tetrad.h"
#include "physics/torsion_field.h"
#include "physics/pec_solver.h"
#include "physics/bounce_solver.h"
#include "physics/soliton.h"
#include "physics/membrane.h"
#include "physics/forman_ricci.h"
#include "thermo/szilard_engine.h"
#include "thermo/generative_model.h"
#include "thermo/markov_blanket.h"
#include "dqfr/dqfr_controller.h"
#include "usf_world.h"

void initialize_usf_engine_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    GDREGISTER_CLASS(USFConstants);
    GDREGISTER_CLASS(SimplicialComplex);
    GDREGISTER_CLASS(DynamicCoherence);
    GDREGISTER_CLASS(LeeWickRegulator);
    GDREGISTER_CLASS(ReggeTensor);
    GDREGISTER_CLASS(Tetrad);
    GDREGISTER_CLASS(TorsionField);
    GDREGISTER_CLASS(PECSolver);
    GDREGISTER_CLASS(BounceSolver);
    GDREGISTER_CLASS(Soliton);
    GDREGISTER_CLASS(HorizonMembrane);
    GDREGISTER_CLASS(FormanRicci);
    GDREGISTER_CLASS(SzilardEngine);
    GDREGISTER_CLASS(GenerativeModel);
    GDREGISTER_CLASS(MarkovBlanket);
    GDREGISTER_CLASS(DQFRController);
    GDREGISTER_CLASS(USFWorld);
}

void uninitialize_usf_engine_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}
