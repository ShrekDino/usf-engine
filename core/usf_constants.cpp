#include "core/usf_constants.h"
#include "core/math/math_funcs.h"
#include "core/math/math_defs.h"

USFConstants::USFConstants() {
    recompute();
}

void USFConstants::recompute() {
    planck_length = planck_length_from(c, g, h_bar);
    planck_time = planck_time_from(c, g, h_bar);
    planck_mass = planck_mass_from(c, g, h_bar);
    planck_energy = planck_mass * c * c;
    planck_temp = planck_energy / k_b;
    planck_density = planck_mass / (planck_length * planck_length * planck_length);
    kappa = kappa_from(g);
    lee_wick_scale = planck_energy / 100.0;
    phi_0 = planck_energy / (planck_length * planck_length * planck_length);
    phi_mass = 1.0e3 * 1.602176634e-7;
}

void USFConstants::set_c(double p_c) {
    c = p_c;
    recompute();
}

void USFConstants::set_g(double p_g) {
    g = p_g;
    recompute();
}

void USFConstants::set_h_bar(double p_h) {
    h_bar = p_h;
    recompute();
}

void USFConstants::set_k_b(double p_kb) {
    k_b = p_kb;
    recompute();
}

void USFConstants::set_alpha(double p_alpha) {
    alpha = p_alpha;
}

void USFConstants::set_phi_0(double p_phi) {
    phi_0 = p_phi;
}

void USFConstants::set_phi_mass(double p_m) {
    phi_mass = p_m;
}

double USFConstants::planck_length_from(double p_c, double p_g, double p_h) {
    return Math::sqrt(p_h * p_g / (p_c * p_c * p_c));
}

double USFConstants::planck_time_from(double p_c, double p_g, double p_h) {
    return Math::sqrt(p_h * p_g / (p_c * p_c * p_c * p_c * p_c));
}

double USFConstants::planck_mass_from(double p_c, double p_g, double p_h) {
    return Math::sqrt(p_h * p_c / p_g);
}

double USFConstants::kappa_from(double p_g) {
    return Math::sqrt(8.0 * Math_PI * p_g);
}

void USFConstants::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_c"), &USFConstants::get_c);
    ClassDB::bind_method(D_METHOD("set_c", "c"), &USFConstants::set_c);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "c", PROPERTY_HINT_RANGE, "1e6,1e9,1e6"), "set_c", "get_c");

    ClassDB::bind_method(D_METHOD("get_g"), &USFConstants::get_g);
    ClassDB::bind_method(D_METHOD("set_g", "g"), &USFConstants::set_g);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "g", PROPERTY_HINT_RANGE, "1e-15,1e-5,1e-12"), "set_g", "get_g");

    ClassDB::bind_method(D_METHOD("get_h_bar"), &USFConstants::get_h_bar);
    ClassDB::bind_method(D_METHOD("set_h_bar", "h_bar"), &USFConstants::set_h_bar);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "h_bar", PROPERTY_HINT_RANGE, "1e-40,1e-20,1e-36"), "set_h_bar", "get_h_bar");

    ClassDB::bind_method(D_METHOD("get_k_b"), &USFConstants::get_k_b);
    ClassDB::bind_method(D_METHOD("set_k_b", "k_b"), &USFConstants::set_k_b);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "k_b", PROPERTY_HINT_RANGE, "1e-30,1e-20,1e-26"), "set_k_b", "get_k_b");

    ClassDB::bind_method(D_METHOD("get_planck_length"), &USFConstants::get_planck_length);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "planck_length", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_planck_length");

    ClassDB::bind_method(D_METHOD("get_planck_time"), &USFConstants::get_planck_time);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "planck_time", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_planck_time");

    ClassDB::bind_method(D_METHOD("get_planck_mass"), &USFConstants::get_planck_mass);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "planck_mass", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_planck_mass");

    ClassDB::bind_method(D_METHOD("get_planck_energy"), &USFConstants::get_planck_energy);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "planck_energy", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_planck_energy");

    ClassDB::bind_method(D_METHOD("get_planck_temp"), &USFConstants::get_planck_temp);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "planck_temp", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_planck_temp");

    ClassDB::bind_method(D_METHOD("get_planck_density"), &USFConstants::get_planck_density);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "planck_density", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_planck_density");

    ClassDB::bind_method(D_METHOD("get_kappa"), &USFConstants::get_kappa);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "kappa", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_kappa");

    ClassDB::bind_method(D_METHOD("get_lee_wick_scale"), &USFConstants::get_lee_wick_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lee_wick_scale", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_lee_wick_scale");

    ClassDB::bind_method(D_METHOD("get_alpha"), &USFConstants::get_alpha);
    ClassDB::bind_method(D_METHOD("set_alpha", "alpha"), &USFConstants::set_alpha);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "alpha", PROPERTY_HINT_RANGE, "0.01,10.0,0.01"), "set_alpha", "get_alpha");

    ClassDB::bind_method(D_METHOD("get_phi_0"), &USFConstants::get_phi_0);
    ClassDB::bind_method(D_METHOD("set_phi_0", "phi_0"), &USFConstants::set_phi_0);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phi_0", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_phi_0", "get_phi_0");

    ClassDB::bind_method(D_METHOD("get_phi_mass"), &USFConstants::get_phi_mass);
    ClassDB::bind_method(D_METHOD("set_phi_mass", "phi_mass"), &USFConstants::set_phi_mass);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phi_mass", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_phi_mass", "get_phi_mass");

    ClassDB::bind_method(D_METHOD("recompute"), &USFConstants::recompute);
}
