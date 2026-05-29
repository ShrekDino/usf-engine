#include "core/lee_wick.h"
#include "core/math/math_funcs.h"

LeeWickRegulator::LeeWickRegulator() {
    constants.instantiate();
}

void LeeWickRegulator::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
}

double LeeWickRegulator::get_min_edge_length() const {
    return constants->planck_length;
}

double LeeWickRegulator::get_lambda() const {
    return constants->lee_wick_scale;
}

double LeeWickRegulator::clamp_edge_length(double raw_length) const {
    double abs_l = raw_length >= 0.0 ? raw_length : -raw_length;
    double min_l = constants->planck_length;
    if (abs_l < min_l) {
        return raw_length >= 0.0 ? min_l : -min_l;
    }
    return raw_length;
}

double LeeWickRegulator::lee_wick_force(double edge_length) const {
    double abs_l = edge_length >= 0.0 ? edge_length : -edge_length;
    double min_l = constants->planck_length;
    if (abs_l < min_l) {
        double delta = min_l - abs_l;
        return alpha * delta * constants->lee_wick_scale;
    }
    return 0.0;
}

double LeeWickRegulator::regulated_volume_term(double deficit_angle, double hinge_volume) const {
    double lambda_sq = constants->lee_wick_scale * constants->lee_wick_scale;
    double eps_sq = deficit_angle * deficit_angle;
    return alpha * eps_sq / (hinge_volume * lambda_sq);
}

double LeeWickRegulator::regulated_action_correction(double regge_action, double avg_curvature) const {
    double lambda_sq = constants->lee_wick_scale * constants->lee_wick_scale;
    double curv_sq = avg_curvature * avg_curvature;
    return alpha * curv_sq / lambda_sq * regge_action;
}

void LeeWickRegulator::set_alpha(double p_alpha) {
    alpha = p_alpha;
}

double LeeWickRegulator::volume_term_potential(double sqrt_g, double sqrt_g0, double phi_0, double p_alpha) {
    double ratio = sqrt_g / sqrt_g0;
    double base = 1.0 + ratio * ratio;
    return -phi_0 * Math::pow(base, -p_alpha * 0.5);
}

void LeeWickRegulator::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &LeeWickRegulator::set_constants);
    ClassDB::bind_method(D_METHOD("get_constants"), &LeeWickRegulator::get_constants);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "constants", PROPERTY_HINT_RESOURCE_TYPE, "USFConstants"), "set_constants", "get_constants");

    ClassDB::bind_method(D_METHOD("set_alpha", "alpha"), &LeeWickRegulator::set_alpha);
    ClassDB::bind_method(D_METHOD("get_alpha"), &LeeWickRegulator::get_alpha);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "alpha", PROPERTY_HINT_RANGE, "0.01,10.0,0.01"), "set_alpha", "get_alpha");

    ClassDB::bind_method(D_METHOD("clamp_edge_length", "raw_length"), &LeeWickRegulator::clamp_edge_length);
    ClassDB::bind_method(D_METHOD("lee_wick_force", "edge_length"), &LeeWickRegulator::lee_wick_force);
    ClassDB::bind_method(D_METHOD("regulated_volume_term", "deficit_angle", "hinge_volume"), &LeeWickRegulator::regulated_volume_term);
}
