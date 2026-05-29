#ifndef MEMBRANE_H
#define MEMBRANE_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "../core/usf_constants.h"

class HorizonMembrane : public Resource {
    GDCLASS(HorizonMembrane, Resource);

protected:
    static void _bind_methods();

public:
    double mass = 0.0;
    double horizon_radius = 0.0;
    double dissipation_scale = 0.0;
    double echo_delay = 0.0;

    Ref<USFConstants> constants;

    HorizonMembrane();

    void set_constants(Ref<USFConstants> p_constants);
    Ref<USFConstants> get_constants() const { return constants; }

    void build(double p_mass, double p_dissipation_scale);

    void recalculate_echo();
    void set_mass(double new_mass);
    double echo_frequency() const;

    double compute_echo_delay(double p_mass, double r_h, double delta_0) const;

    Ref<HorizonMembrane> merger_ringdown(Ref<HorizonMembrane> other);

    double get_mass() const { return mass; }
    double get_horizon_radius() const { return horizon_radius; }
    double get_dissipation_scale() const { return dissipation_scale; }
    double get_echo_delay() const { return echo_delay; }
};

#endif // MEMBRANE_H
