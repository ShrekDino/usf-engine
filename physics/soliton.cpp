#include "core/math/math_defs.h"
#include "soliton.h"
#include "core/math/math_funcs.h"
#include <cmath>

Soliton::Soliton() {
    constants.instantiate();
}

void Soliton::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
}

void Soliton::build(double p_mass, int charge) {
    mass = p_mass;
    topological_charge = charge;
    horizon_radius = 2.0 * constants->g * mass / (constants->c * constants->c);
    core_density = mass / ((4.0 / 3.0) * Math::PI * horizon_radius * horizon_radius * horizon_radius);

    int n_points = 100;
    pressure_profile.resize(n_points);
    for (int i = 0; i < n_points; i++) {
        double r = ((double)i / (double)n_points) * horizon_radius * 3.0;
        double p;
        if (r < horizon_radius) {
            double ratio = r / horizon_radius;
            p = core_density * constants->c * constants->c * (1.0 - ratio * ratio);
        } else {
            double ratio = (r - horizon_radius) / horizon_radius;
            p = core_density * constants->c * constants->c * std::exp(-ratio * ratio);
        }
        pressure_profile.set(i, p);
    }
}

double Soliton::bps_mass() const {
    double vol = (4.0 / 3.0) * Math::PI * horizon_radius * horizon_radius * horizon_radius;
    return core_density * vol;
}

double Soliton::mass_defect() const {
    return mass - bps_mass();
}

double Soliton::compactness() const {
    double r_s = 2.0 * constants->g * mass / (constants->c * constants->c);
    return r_s / horizon_radius;
}

double Soliton::central_pressure() const {
    if (pressure_profile.size() > 0) {
        return pressure_profile[0];
    }
    return 0.0;
}

double Soliton::pressure_at(double r_fraction) const {
    if (pressure_profile.size() == 0) {
        return 0.0;
    }
    int idx = (int)(r_fraction * (double)(pressure_profile.size() - 1));
    if (idx >= pressure_profile.size()) {
        idx = pressure_profile.size() - 1;
    }
    if (idx < 0) {
        idx = 0;
    }
    return pressure_profile[idx];
}

bool Soliton::topological_stability() const {
    double defect = mass_defect();
    if (defect < 0.0) {
        defect = -defect;
    }
    return topological_charge != 0 && defect < mass * 0.5;
}

void Soliton::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &Soliton::set_constants);
    ClassDB::bind_method(D_METHOD("get_constants"), &Soliton::get_constants);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "constants", PROPERTY_HINT_RESOURCE_TYPE, "USFConstants"), "set_constants", "get_constants");

    ClassDB::bind_method(D_METHOD("build", "mass", "charge"), &Soliton::build);

    ClassDB::bind_method(D_METHOD("bps_mass"), &Soliton::bps_mass);
    ClassDB::bind_method(D_METHOD("mass_defect"), &Soliton::mass_defect);
    ClassDB::bind_method(D_METHOD("compactness"), &Soliton::compactness);
    ClassDB::bind_method(D_METHOD("central_pressure"), &Soliton::central_pressure);
    ClassDB::bind_method(D_METHOD("pressure_at", "r_fraction"), &Soliton::pressure_at);
    ClassDB::bind_method(D_METHOD("topological_stability"), &Soliton::topological_stability);

    ClassDB::bind_method(D_METHOD("get_mass"), &Soliton::get_mass);
    ClassDB::bind_method(D_METHOD("get_topological_charge"), &Soliton::get_topological_charge);
    ClassDB::bind_method(D_METHOD("get_horizon_radius"), &Soliton::get_horizon_radius);
    ClassDB::bind_method(D_METHOD("get_core_density"), &Soliton::get_core_density);
    ClassDB::bind_method(D_METHOD("get_profile_size"), &Soliton::get_profile_size);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mass", PROPERTY_HINT_RANGE, "1e20,1e40,1e30"), "", "get_mass");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "topological_charge", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_topological_charge");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "horizon_radius", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_horizon_radius");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "core_density", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_core_density");
}
