#ifndef PEC_SOLVER_H
#define PEC_SOLVER_H

#include "scene/main/node.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/simplicial_complex.h"
#include "core/regge_tensor.h"
#include "core/usf_constants.h"
#include "physics/tetrad.h"
#include "physics/torsion_field.h"

class PECSolver : public Node {
    GDCLASS(PECSolver, Node);

protected:
    static void _bind_methods();

public:
    Ref<USFConstants> constants;
    Ref<SimplicialComplex> complex;
    Vector<Ref<Tetrad>> tetrads;
    Vector<Ref<TorsionField>> torsion_fields;
    Ref<ReggeTensor> regge;
    double convergence = 0.0;
    int iteration = 0;

    PECSolver();

    void set_complex(Ref<SimplicialComplex> p_complex);
    Ref<SimplicialComplex> get_complex() const { return complex; }

    void set_constants(Ref<USFConstants> p_constants);
    Ref<USFConstants> get_constants() const { return constants; }

    double solve_step();
    bool solve(int max_iterations, double tolerance);

    double einstein_tensor_component(int mu, int nu) const;
    double stress_energy_tensor_component(int mu, int nu) const;

    double get_convergence() const { return convergence; }
    int get_iteration() const { return iteration; }
    void reset();
};

#endif // PEC_SOLVER_H
