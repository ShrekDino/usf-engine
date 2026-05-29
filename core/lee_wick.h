#ifndef LEE_WICK_H
#define LEE_WICK_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/usf_constants.h"

class LeeWickRegulator : public Resource {
    GDCLASS(LeeWickRegulator, Resource);

protected:
    static void _bind_methods();

public:
    Ref<USFConstants> constants;
    double alpha = 1.0;

    LeeWickRegulator();

    void set_constants(Ref<USFConstants> p_constants);
    Ref<USFConstants> get_constants() const { return constants; }

    double get_min_edge_length() const;
    double get_lambda() const;

    double clamp_edge_length(double raw_length) const;
    double lee_wick_force(double edge_length) const;

    double regulated_volume_term(double deficit_angle, double hinge_volume) const;
    double regulated_action_correction(double regge_action, double avg_curvature) const;

    double get_alpha() const { return alpha; }
    void set_alpha(double p_alpha);

    static double volume_term_potential(double sqrt_g, double sqrt_g0, double phi_0, double p_alpha);
};

#endif // LEE_WICK_H
