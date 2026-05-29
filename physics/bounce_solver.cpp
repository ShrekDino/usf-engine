#include "core/math/math_defs.h"
#include "bounce_solver.h"
#include "core/math/math_funcs.h"
#include <cmath>

BounceSolver::BounceSolver() {
    constants.instantiate();
}

void BounceSolver::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
}

void BounceSolver::initialize(double init_scale, double init_density, double init_torsion_s2, double curvature) {
    a = init_scale;
    rho_matter = init_density;
    torsion_s2 = init_torsion_s2;
    k_curvature = curvature;
    time = 0.0;
    bounce_detected = false;
    a_min = init_scale;
    time_of_bounce = 0.0;

    double g = constants->g;
    double kappa = constants->kappa;

    double h_sq = 8.0 * Math::PI * g * rho_matter / 3.0
                  + 3.0 * kappa * kappa * torsion_s2
                  - k_curvature / (a * a);
    h = h_sq > 0.0 ? std::sqrt(h_sq) : 0.0;
}

void BounceSolver::step(double dt) {
    double c = constants->c;
    double g = constants->g;
    double kappa = constants->kappa;
    double lw_min = constants->planck_length;

    double a_current = a;

    double rho_m = rho_matter;
    double p_m = 0.0;

    double rho_torsion = 1.5 * kappa * kappa * torsion_s2;
    double p_torsion = rho_torsion;

    double total_rho = rho_m + rho_torsion;
    double total_p = p_m + p_torsion;

    double a_ddot = -(4.0 * Math::PI * g / (3.0 * c * c))
                    * (total_rho + 3.0 * total_p / (c * c))
                    * a_current;

    double torsion_boost = 3.0 * kappa * kappa * torsion_s2 / a_current;

    double a_ddot_total = a_ddot + torsion_boost;

    h = a_ddot_total * dt + h;
    if (h < 0.0) {
        h = 0.0;
    }

    double a_new = a_current + h * dt + 0.5 * a_ddot_total * dt * dt;
    if (a_new < lw_min) {
        a_new = lw_min;
    }

    double rho_m_new = rho_m * std::pow(a_current / a_new, 3.0);
    double torsion_s2_new = torsion_s2 * std::pow(a_current / a_new, 6.0);

    a = a_new;
    rho_matter = rho_m_new;
    torsion_s2 = torsion_s2_new;

    if (!bounce_detected && h < 0.01 && a_new < a_min * 1.01) {
        double h_sq = 8.0 * Math::PI * g * rho_m_new / 3.0
                      + 3.0 * kappa * kappa * torsion_s2_new
                      - k_curvature / (a_new * a_new);
        double h_min = h_sq > 0.0 ? std::sqrt(h_sq) : 0.0;

        if (h_min < 1e-6) {
            bounce_detected = true;
            time_of_bounce = time;
            a_min = a_new;
        }
    }

    if (a_new < a_min) {
        a_min = a_new;
    }

    double h_sq = 8.0 * Math::PI * g * rho_m_new / 3.0
                  + 3.0 * kappa * kappa * torsion_s2_new
                  - k_curvature / (a_new * a_new);
    h = h_sq > 0.0 ? std::sqrt(h_sq) : 0.0;

    time += dt;
}

void BounceSolver::integrate(int steps, double dt) {
    for (int i = 0; i < steps; i++) {
        step(dt);
        if (bounce_detected && a > a_min * 2.0) {
            break;
        }
    }
}

double BounceSolver::hubble_squared() const {
    return 8.0 * Math::PI * constants->g * rho_matter / 3.0
           + 3.0 * constants->kappa * constants->kappa * torsion_s2
           - k_curvature / (a * a);
}

bool BounceSolver::torsion_dominated() const {
    double g_term = 8.0 * Math::PI * constants->g * rho_matter / 3.0;
    double t_term = 3.0 * constants->kappa * constants->kappa * torsion_s2;
    return t_term > g_term;
}

double BounceSolver::raychaudhuri_term() const {
    double kappa = constants->kappa;
    double total_rho = rho_matter + (1.5 * kappa * kappa * torsion_s2);
    return -(4.0 * Math::PI * constants->g / (3.0 * constants->c * constants->c))
           * (total_rho + 3.0 * total_rho / (constants->c * constants->c));
}

double BounceSolver::torsion_repulsion_term() const {
    return 3.0 * constants->kappa * constants->kappa * torsion_s2 / a;
}

void BounceSolver::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &BounceSolver::set_constants);
    ClassDB::bind_method(D_METHOD("get_constants"), &BounceSolver::get_constants);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "constants", PROPERTY_HINT_RESOURCE_TYPE, "USFConstants"), "set_constants", "get_constants");

    ClassDB::bind_method(D_METHOD("initialize", "init_scale", "init_density", "init_torsion_s2", "curvature"), &BounceSolver::initialize);
    ClassDB::bind_method(D_METHOD("step", "dt"), &BounceSolver::step);
    ClassDB::bind_method(D_METHOD("integrate", "steps", "dt"), &BounceSolver::integrate);

    ClassDB::bind_method(D_METHOD("get_scale_factor"), &BounceSolver::get_scale_factor);
    ClassDB::bind_method(D_METHOD("get_hubble"), &BounceSolver::get_hubble);
    ClassDB::bind_method(D_METHOD("get_rho_matter"), &BounceSolver::get_rho_matter);
    ClassDB::bind_method(D_METHOD("get_torsion_s2"), &BounceSolver::get_torsion_s2);
    ClassDB::bind_method(D_METHOD("get_time"), &BounceSolver::get_time);
    ClassDB::bind_method(D_METHOD("get_a_min"), &BounceSolver::get_a_min);
    ClassDB::bind_method(D_METHOD("get_time_of_bounce"), &BounceSolver::get_time_of_bounce);
    ClassDB::bind_method(D_METHOD("get_bounce_detected"), &BounceSolver::get_bounce_detected);

    ClassDB::bind_method(D_METHOD("hubble_squared"), &BounceSolver::hubble_squared);
    ClassDB::bind_method(D_METHOD("torsion_dominated"), &BounceSolver::torsion_dominated);
    ClassDB::bind_method(D_METHOD("raychaudhuri_term"), &BounceSolver::raychaudhuri_term);
    ClassDB::bind_method(D_METHOD("torsion_repulsion_term"), &BounceSolver::torsion_repulsion_term);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scale_factor", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_scale_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hubble", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_hubble");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rho_matter", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_rho_matter");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "a_min", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_a_min");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bounce_detected", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_bounce_detected");
}
