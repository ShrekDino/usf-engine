#include "physics/tetrad.h"
#include "core/math/math_funcs.h"
#include <cmath>

Tetrad::Tetrad() {
    for (int a = 0; a < 4; a++) {
        for (int mu = 0; mu < 4; mu++) {
            frame[a][mu] = (a == mu) ? 1.0 : 0.0;
            coframe[a][mu] = (a == mu) ? 1.0 : 0.0;
        }
    }
}

Ref<Tetrad> Tetrad::minkowski() {
    Ref<Tetrad> t;
    t.instantiate();
    return t;
}

void Tetrad::set_frame(int a, int mu, double val) {
    if (a >= 0 && a < 4 && mu >= 0 && mu < 4) {
        frame[a][mu] = val;
    }
}

double Tetrad::get_frame(int a, int mu) const {
    if (a >= 0 && a < 4 && mu >= 0 && mu < 4) {
        return frame[a][mu];
    }
    return 0.0;
}

void Tetrad::get_metric(double g[4][4]) const {
    double eta[4] = { -1.0, 1.0, 1.0, 1.0 };
    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            g[mu][nu] = 0.0;
            for (int a = 0; a < 4; a++) {
                g[mu][nu] += frame[a][mu] * frame[a][nu] * eta[a];
            }
        }
    }
}

double Tetrad::metric_component(int mu, int nu) const {
    double eta[4] = { -1.0, 1.0, 1.0, 1.0 };
    double val = 0.0;
    for (int a = 0; a < 4; a++) {
        val += frame[a][mu] * frame[a][nu] * eta[a];
    }
    return val;
}

void Tetrad::get_inverse_metric(double g_inv[4][4]) const {
    double eta[4] = { -1.0, 1.0, 1.0, 1.0 };
    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            g_inv[mu][nu] = 0.0;
            for (int a = 0; a < 4; a++) {
                g_inv[mu][nu] += coframe[a][mu] * coframe[a][nu] * eta[a];
            }
        }
    }
}

double Tetrad::inverse_metric_component(int mu, int nu) const {
    double eta[4] = { -1.0, 1.0, 1.0, 1.0 };
    double val = 0.0;
    for (int a = 0; a < 4; a++) {
        val += coframe[a][mu] * coframe[a][nu] * eta[a];
    }
    return val;
}

double Tetrad::det3x3(const double m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
           - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
           + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

double Tetrad::det4x4(const double m[4][4]) {
    double det = 0.0;
    for (int i = 0; i < 4; i++) {
        double sub[3][3];
        for (int j = 1; j < 4; j++) {
            int col = 0;
            for (int k = 0; k < 4; k++) {
                if (k == i) continue;
                sub[j - 1][col] = m[j][k];
                col++;
            }
        }
        double s = (i % 2 == 0) ? 1.0 : -1.0;
        det += s * m[0][i] * det3x3(sub);
    }
    return det;
}

bool Tetrad::invert4x4(const double m[4][4], double inv[4][4]) {
    double det = det4x4(m);
    if (std::abs(det) < 1e-30) {
        return false;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            double sub[3][3];
            int si = 0;
            for (int ii = 0; ii < 4; ii++) {
                if (ii == i) continue;
                int sj = 0;
                for (int jj = 0; jj < 4; jj++) {
                    if (jj == j) continue;
                    sub[si][sj] = m[ii][jj];
                    sj++;
                }
                si++;
            }
            double s = ((i + j) % 2 == 0) ? 1.0 : -1.0;
            inv[j][i] = s * det3x3(sub) / det;
        }
    }
    return true;
}

double Tetrad::determinant() const {
    double g[4][4];
    get_metric(g);
    return det4x4(g);
}

void Tetrad::compute_coframe() {
    double g[4][4];
    get_metric(g);
    double det = det4x4(g);
    if (std::abs(det) < 1e-30) {
        return;
    }

    for (int a = 0; a < 4; a++) {
        for (int nu = 0; nu < 4; nu++) {
            double val = 0.0;
            for (int mu = 0; mu < 4; mu++) {
                val += g[mu][nu] * frame[a][mu];
            }
            coframe[a][nu] = val;
        }
    }
}

void Tetrad::perturb(double amplitude) {
    for (int a = 0; a < 4; a++) {
        for (int mu = 0; mu < 4; mu++) {
            frame[a][mu] += ((double)rand() / RAND_MAX - 0.5) * 2.0 * amplitude;
        }
    }
}

void Tetrad::randomize(double amplitude) {
    for (int a = 0; a < 4; a++) {
        for (int mu = 0; mu < 4; mu++) {
            frame[a][mu] = ((double)rand() / RAND_MAX - 0.5) * 2.0 * amplitude;
            if (a == mu) {
                frame[a][mu] += 1.0;
            }
        }
    }
    compute_coframe();
}

double Tetrad::levi_civita_symbol(int mu, int nu, int rho, int sigma) const {
    if (mu == nu || mu == rho || mu == sigma || nu == rho || nu == sigma || rho == sigma) {
        return 0.0;
    }

    int perm[4] = { mu, nu, rho, sigma };
    int sign = 1;
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (perm[i] > perm[j]) {
                sign = -sign;
            }
        }
    }

    return (double)sign;
}

void Tetrad::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_frame", "a", "mu", "val"), &Tetrad::set_frame);
    ClassDB::bind_method(D_METHOD("get_frame", "a", "mu"), &Tetrad::get_frame);

    ClassDB::bind_method(D_METHOD("metric_component", "mu", "nu"), &Tetrad::metric_component);
    ClassDB::bind_method(D_METHOD("inverse_metric_component", "mu", "nu"), &Tetrad::inverse_metric_component);
    ClassDB::bind_method(D_METHOD("determinant"), &Tetrad::determinant);
    ClassDB::bind_method(D_METHOD("compute_coframe"), &Tetrad::compute_coframe);
    ClassDB::bind_method(D_METHOD("perturb", "amplitude"), &Tetrad::perturb);

    ClassDB::bind_static_method("Tetrad", D_METHOD("minkowski"), &Tetrad::minkowski);
}
