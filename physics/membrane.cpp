#include "membrane.h"
#include "core/math/math_funcs.h"
#include <cmath>

HorizonMembrane::HorizonMembrane() :
        dissipation_scale(0.0),
        echo_delay(0.0) {
    constants.instantiate();
}

void HorizonMembrane::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
}

void HorizonMembrane::build(double p_mass, double p_dissipation_scale) {
    mass = p_mass;
    horizon_radius = 2.0 * constants->g * mass / (constants->c * constants->c);
    if (p_dissipation_scale <= 0.0) {
        dissipation_scale = constants->planck_length;
    } else {
        dissipation_scale = p_dissipation_scale;
    }
    recalculate_echo();
}

void HorizonMembrane::recalculate_echo() {
    echo_delay = compute_echo_delay(mass, horizon_radius, dissipation_scale);
}

void HorizonMembrane::set_mass(double new_mass) {
    mass = new_mass;
    horizon_radius = 2.0 * constants->g * mass / (constants->c * constants->c);
    recalculate_echo();
}

double HorizonMembrane::echo_frequency() const {
    if (echo_delay <= 0.0) {
        return 0.0;
    }
    return 1.0 / echo_delay;
}

double HorizonMembrane::compute_echo_delay(double p_mass, double r_h, double delta_0) const {
    double c = constants->c;
    double g = constants->g;
    double ratio = r_h / delta_0;
    if (ratio <= 1.0) {
        return 0.0;
    }
    return (4.0 * g * p_mass / (c * c * c)) * std::log(ratio);
}

Ref<HorizonMembrane> HorizonMembrane::merger_ringdown(Ref<HorizonMembrane> other) {
    double new_mass = mass + other->mass;
    Ref<HorizonMembrane> merged;
    merged.instantiate();
    merged->set_constants(constants);
    merged->build(new_mass, dissipation_scale);
    return merged;
}

void HorizonMembrane::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &HorizonMembrane::set_constants);
    ClassDB::bind_method(D_METHOD("get_constants"), &HorizonMembrane::get_constants);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "constants", PROPERTY_HINT_RESOURCE_TYPE, "USFConstants"), "set_constants", "get_constants");

    ClassDB::bind_method(D_METHOD("build", "mass", "dissipation_scale"), &HorizonMembrane::build);
    ClassDB::bind_method(D_METHOD("recalculate_echo"), &HorizonMembrane::recalculate_echo);
    ClassDB::bind_method(D_METHOD("set_mass", "new_mass"), &HorizonMembrane::set_mass);
    ClassDB::bind_method(D_METHOD("echo_frequency"), &HorizonMembrane::echo_frequency);

    ClassDB::bind_method(D_METHOD("get_mass"), &HorizonMembrane::get_mass);
    ClassDB::bind_method(D_METHOD("get_horizon_radius"), &HorizonMembrane::get_horizon_radius);
    ClassDB::bind_method(D_METHOD("get_dissipation_scale"), &HorizonMembrane::get_dissipation_scale);
    ClassDB::bind_method(D_METHOD("get_echo_delay"), &HorizonMembrane::get_echo_delay);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mass", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_mass");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "horizon_radius", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_horizon_radius");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dissipation_scale", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_dissipation_scale");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "echo_delay", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_echo_delay");
}
