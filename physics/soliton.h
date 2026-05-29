#ifndef SOLITON_H
#define SOLITON_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/usf_constants.h"

class Soliton : public Resource {
    GDCLASS(Soliton, Resource);

protected:
    static void _bind_methods();

public:
    double mass = 0.0;
    int topological_charge = 0;
    double horizon_radius = 0.0;
    Vector<double> pressure_profile;
    double core_density = 0.0;

    Ref<USFConstants> constants;

    Soliton();

    void set_constants(Ref<USFConstants> p_constants);
    Ref<USFConstants> get_constants() const { return constants; }

    void build(double p_mass, int charge);

    double bps_mass() const;
    double mass_defect() const;
    double compactness() const;
    double central_pressure() const;
    double pressure_at(double r_fraction) const;
    bool topological_stability() const;

    double get_mass() const { return mass; }
    int get_topological_charge() const { return topological_charge; }
    double get_horizon_radius() const { return horizon_radius; }
    double get_core_density() const { return core_density; }
    int get_profile_size() const { return pressure_profile.size(); }
};

#endif // SOLITON_H
