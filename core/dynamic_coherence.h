#ifndef DYNAMIC_COHERENCE_H
#define DYNAMIC_COHERENCE_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "simplicial_complex.h"

class DynamicCoherence : public Resource {
    GDCLASS(DynamicCoherence, Resource);

protected:
    static void _bind_methods();

public:
    double omega_0 = 1.0;
    double omega_coherence = 1.0;
    double window_exponent = 2.0;

    DynamicCoherence();

    void set_base_radius(double p_omega_0);
    double get_base_radius() const { return omega_0; }

    void recompute_from_ricci(double integrated_ricci_curvature);
    double gaussian_window(double distance_sq) const;
    double interaction_weight(const Vec4d &a, const Vec4d &b) const;

    double get_omega_coherence() const { return omega_coherence; }
    double get_window_exponent() const { return window_exponent; }
    void set_window_exponent(double p_exp) { window_exponent = p_exp; }

    bool is_within_coherence(double distance_sq) const;
};

#endif // DYNAMIC_COHERENCE_H
