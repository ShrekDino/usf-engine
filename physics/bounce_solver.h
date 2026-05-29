#ifndef BOUNCE_SOLVER_H
#define BOUNCE_SOLVER_H

#include "scene/main/node.h"
#include "core/object/class_db.h"
#include "../core/usf_constants.h"

class BounceSolver : public Node {
    GDCLASS(BounceSolver, Node);

protected:
    static void _bind_methods();

public:
    double a = 1.0;
    double rho_matter = 0.0;
    double torsion_s2 = 0.0;
    double h = 0.0;
    double time = 0.0;
    double k_curvature = 0.0;
    bool bounce_detected = false;
    double a_min = 1.0;
    double time_of_bounce = 0.0;

    Ref<USFConstants> constants;

    BounceSolver();

    void set_constants(Ref<USFConstants> p_constants);
    Ref<USFConstants> get_constants() const { return constants; }

    void initialize(double init_scale, double init_density, double init_torsion_s2, double curvature);

    void step(double dt);
    void integrate(int steps, double dt);

    double hubble_squared() const;
    bool torsion_dominated() const;

    double get_scale_factor() const { return a; }
    double get_hubble() const { return h; }
    double get_rho_matter() const { return rho_matter; }
    double get_torsion_s2() const { return torsion_s2; }
    double get_time() const { return time; }
    double get_a_min() const { return a_min; }
    double get_time_of_bounce() const { return time_of_bounce; }
    bool get_bounce_detected() const { return bounce_detected; }

    double raychaudhuri_term() const;
    double torsion_repulsion_term() const;
};

#endif // BOUNCE_SOLVER_H
