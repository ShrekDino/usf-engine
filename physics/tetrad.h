#ifndef TETRAD_H
#define TETRAD_H

#include "core/io/resource.h"
#include "core/object/class_db.h"

class Tetrad : public Resource {
    GDCLASS(Tetrad, Resource);

protected:
    static void _bind_methods();

public:
    double frame[4][4];
    double coframe[4][4];

    Tetrad();

    static Ref<Tetrad> minkowski();

    void set_frame(int a, int mu, double val);
    double get_frame(int a, int mu) const;

    void get_metric(double g[4][4]) const;
    double metric_component(int mu, int nu) const;
    void get_inverse_metric(double g_inv[4][4]) const;
    double inverse_metric_component(int mu, int nu) const;

    double determinant() const;
    void compute_coframe();

    void perturb(double amplitude);
    void randomize(double amplitude);

    double levi_civita_symbol(int mu, int nu, int rho, int sigma) const;

private:
    static double det3x3(const double m[3][3]);
    static double det4x4(const double m[4][4]);
    static bool invert4x4(const double m[4][4], double inv[4][4]);
};

#endif // TETRAD_H
