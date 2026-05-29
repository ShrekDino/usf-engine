#include "core/dynamic_coherence.h"
#include "core/math/math_funcs.h"

DynamicCoherence::DynamicCoherence() {
}

void DynamicCoherence::set_base_radius(double p_omega_0) {
    omega_0 = p_omega_0;
    omega_coherence = omega_0;
}

void DynamicCoherence::recompute_from_ricci(double integrated_ricci_curvature) {
    omega_coherence = omega_0 * Math::exp(-integrated_ricci_curvature);
    if (omega_coherence < 1e-12 * omega_0) {
        omega_coherence = 1e-12 * omega_0;
    }
}

double DynamicCoherence::gaussian_window(double distance_sq) const {
    if (omega_coherence <= 0.0) {
        return 0.0;
    }
    double norm = distance_sq / (omega_coherence * omega_coherence);
    return Math::exp(-Math::pow(norm, window_exponent * 0.5));
}

double DynamicCoherence::interaction_weight(const Vec4d &a, const Vec4d &b) const {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    double dz = a.z - b.z;
    double dw = a.w - b.w;
    double dist_sq = dx * dx + dy * dy + dz * dz + dw * dw;
    return gaussian_window(dist_sq);
}

bool DynamicCoherence::is_within_coherence(double distance_sq) const {
    if (omega_coherence <= 0.0) {
        return false;
    }
    double cutoff_sq = omega_coherence * omega_coherence;
    return distance_sq <= cutoff_sq;
}

void DynamicCoherence::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_base_radius", "radius"), &DynamicCoherence::set_base_radius);
    ClassDB::bind_method(D_METHOD("get_base_radius"), &DynamicCoherence::get_base_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_radius", PROPERTY_HINT_RANGE, "1e-40,1e30,1e-10"), "set_base_radius", "get_base_radius");

    ClassDB::bind_method(D_METHOD("get_omega_coherence"), &DynamicCoherence::get_omega_coherence);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "omega_coherence", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_omega_coherence");

    ClassDB::bind_method(D_METHOD("set_window_exponent", "exp"), &DynamicCoherence::set_window_exponent);
    ClassDB::bind_method(D_METHOD("get_window_exponent"), &DynamicCoherence::get_window_exponent);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "window_exponent", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_window_exponent", "get_window_exponent");

    ClassDB::bind_method(D_METHOD("gaussian_window", "distance_sq"), &DynamicCoherence::gaussian_window);
    ClassDB::bind_method(D_METHOD("is_within_coherence", "distance_sq"), &DynamicCoherence::is_within_coherence);
}
