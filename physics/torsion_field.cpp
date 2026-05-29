#include "physics/torsion_field.h"
#include "core/math/math_funcs.h"
#include <cmath>

TorsionField::TorsionField() {
    clear();
}

void TorsionField::clear() {
    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            for (int a = 0; a < 4; a++) {
                spin_connection[mu][nu][a] = 0.0;
                torsion_tensor[mu][nu][a] = 0.0;
                contorsion[mu][nu][a] = 0.0;
            }
        }
        axial_vector[mu] = 0.0;
    }
    torsion_scalar = 0.0;
    torsion_energy_density = 0.0;
    axial_squared = 0.0;
}

void TorsionField::compute(Ref<Tetrad> tetrad, Ref<USFConstants> constants) {
    double dx = 1e-8;
    double spring_const = constants->kappa * constants->kappa;

    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            for (int a = 0; a < 4; a++) {
                double de_mu_nu = partial_derivative(tetrad, a, mu, nu, dx);
                double de_nu_mu = partial_derivative(tetrad, a, nu, mu, dx);

                double omega_term = 0.0;
                for (int b = 0; b < 4; b++) {
                    omega_term += spin_connection[mu][a][b] * tetrad->frame[b][nu]
                                  - spin_connection[nu][a][b] * tetrad->frame[b][mu];
                }

                torsion_tensor[mu][nu][a] = de_mu_nu - de_nu_mu + omega_term;
            }
        }
    }

    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            for (int a = 0; a < 4; a++) {
                contorsion[mu][nu][a] = 0.5 * (torsion_tensor[mu][nu][a]
                                               + torsion_tensor[nu][mu][a]
                                               - torsion_tensor[mu][nu][a]);
            }
        }
    }

    double s2 = 0.0;
    for (int a = 0; a < 4; a++) {
        for (int b = 0; b < 4; b++) {
            for (int c = 0; c < 4; c++) {
                s2 += torsion_tensor[a][b][c] * torsion_tensor[a][b][c];
            }
        }
    }
    torsion_scalar = s2;
    torsion_energy_density = 1.5 * spring_const * s2;

    extract_axial_vector(tetrad);
}

double TorsionField::partial_derivative(const Ref<Tetrad> &tetrad, int a, int coord, int index, double dx) const {
    double x_plus = tetrad->frame[a][index] + dx;
    double x_minus = tetrad->frame[a][index] - dx;
    double f_plus = x_plus;
    double f_minus = x_minus;
    return (f_plus - f_minus) / (2.0 * dx);
}

void TorsionField::extract_axial_vector(const Ref<Tetrad> &tetrad) {
    double g[4][4];
    tetrad->get_metric(g);

    for (int sigma = 0; sigma < 4; sigma++) {
        axial_vector[sigma] = 0.0;
        for (int mu = 0; mu < 4; mu++) {
            for (int nu = 0; nu < 4; nu++) {
                for (int rho = 0; rho < 4; rho++) {
                    int perm[4] = { sigma, mu, nu, rho };
                    bool duplicate = false;
                    for (int p = 0; p < 4; p++) {
                        for (int q = p + 1; q < 4; q++) {
                            if (perm[p] == perm[q]) {
                                duplicate = true;
                                break;
                            }
                        }
                        if (duplicate) break;
                    }
                    if (duplicate) continue;

                    int sign = 1;
                    for (int p = 0; p < 4; p++) {
                        for (int q = p + 1; q < 4; q++) {
                            if (perm[p] > perm[q]) {
                                sign = -sign;
                            }
                        }
                    }

                    double T_upper = 0.0;
                    for (int a = 0; a < 4; a++) {
                        T_upper += g[rho][a] * torsion_tensor[mu][nu][a];
                    }

                    axial_vector[sigma] += sign * T_upper;
                }
            }
        }
        axial_vector[sigma] /= 6.0;
    }

    axial_squared = 0.0;
    for (int mu = 0; mu < 4; mu++) {
        double term = 0.0;
        for (int nu = 0; nu < 4; nu++) {
            term += g[mu][nu] * axial_vector[nu];
        }
        axial_squared += axial_vector[mu] * term;
    }
}

double TorsionField::get_axial_component(int mu) const {
    if (mu >= 0 && mu < 4) {
        return axial_vector[mu];
    }
    return 0.0;
}

double TorsionField::torsion_repulsion(double scale_factor) const {
    if (scale_factor <= 0.0) {
        return 1e300;
    }
    return torsion_energy_density / (scale_factor * scale_factor * scale_factor
                                     * scale_factor * scale_factor * scale_factor);
}

void TorsionField::_bind_methods() {
    ClassDB::bind_method(D_METHOD("compute", "tetrad", "constants"), &TorsionField::compute);
    ClassDB::bind_method(D_METHOD("clear"), &TorsionField::clear);

    ClassDB::bind_method(D_METHOD("get_torsion_scalar"), &TorsionField::get_torsion_scalar);
    ClassDB::bind_method(D_METHOD("get_torsion_energy_density"), &TorsionField::get_torsion_energy_density);
    ClassDB::bind_method(D_METHOD("get_axial_component", "mu"), &TorsionField::get_axial_component);
    ClassDB::bind_method(D_METHOD("get_axial_squared"), &TorsionField::get_axial_squared);

    ClassDB::bind_method(D_METHOD("torsion_repulsion", "scale_factor"), &TorsionField::torsion_repulsion);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "torsion_scalar", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_torsion_scalar");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "torsion_energy_density", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_torsion_energy_density");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "axial_squared", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_axial_squared");
}
