#ifndef USF_CONSTANTS_H
#define USF_CONSTANTS_H

#include "core/io/resource.h"
#include "core/object/class_db.h"

class USFConstants : public Resource {
    GDCLASS(USFConstants, Resource);

protected:
    static void _bind_methods();

public:
    double c = 299792458.0;
    double g = 6.67430e-11;
    double h_bar = 1.054571817e-34;
    double k_b = 1.380649e-23;

    double planck_length = 0.0;
    double planck_time = 0.0;
    double planck_mass = 0.0;
    double planck_energy = 0.0;
    double planck_temp = 0.0;
    double planck_density = 0.0;

    double kappa = 0.0;
    double lee_wick_scale = 0.0;

    double alpha = 1.0;
    double phi_0 = 0.0;
    double phi_mass = 0.0;

    USFConstants();
    void recompute();

    double get_c() const { return c; }
    void set_c(double p_c);

    double get_g() const { return g; }
    void set_g(double p_g);

    double get_h_bar() const { return h_bar; }
    void set_h_bar(double p_h);

    double get_k_b() const { return k_b; }
    void set_k_b(double p_kb);

    double get_planck_length() const { return planck_length; }
    double get_planck_time() const { return planck_time; }
    double get_planck_mass() const { return planck_mass; }
    double get_planck_energy() const { return planck_energy; }
    double get_planck_temp() const { return planck_temp; }
    double get_planck_density() const { return planck_density; }

    double get_kappa() const { return kappa; }
    double get_lee_wick_scale() const { return lee_wick_scale; }

    double get_alpha() const { return alpha; }
    void set_alpha(double p_alpha);

    double get_phi_0() const { return phi_0; }
    void set_phi_0(double p_phi);

    double get_phi_mass() const { return phi_mass; }
    void set_phi_mass(double p_m);

    static double planck_length_from(double p_c, double p_g, double p_h);
    static double planck_time_from(double p_c, double p_g, double p_h);
    static double planck_mass_from(double p_c, double p_g, double p_h);
    static double kappa_from(double p_g);
};

#endif // USF_CONSTANTS_H
