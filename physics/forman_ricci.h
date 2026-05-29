#ifndef FORMAN_RICCI_H
#define FORMAN_RICCI_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/simplicial_complex.h"
#include "core/usf_constants.h"

class FormanRicci : public Resource {
    GDCLASS(FormanRicci, Resource);

protected:
    static void _bind_methods();

public:
    double damping_coefficient = 1.0;
    double gamma_0 = 0.01;
    double I_0 = 1.0;
    double thermostat_alpha = 1.0;
    double noise_temperature = 0.0;
    double planck_floor = 0.0;

    Ref<USFConstants> constants;

    FormanRicci();

    void set_constants(Ref<USFConstants> p_constants);
    Ref<USFConstants> get_constants() const { return constants; }

    Vector<double> compute_edge_curvatures(Ref<SimplicialComplex> complex) const;

    double edge_weight(double edge_length) const;
    double face_weight(double edge_a, double edge_b, double edge_c) const;

    double compute_ricci_for_edge(const SimplicialComplex &complex, int edge_idx) const;
    double compute_ricci_from_vertices(const SimplicialComplex &complex, int v0, int v1) const;

    void damp_metric(Ref<SimplicialComplex> complex, double dt);

    double topological_thermostat(double signal_to_noise) const;

    double get_damping_coefficient() const { return damping_coefficient; }
    void set_damping_coefficient(double val) { damping_coefficient = val; }

    double get_noise_temperature() const { return noise_temperature; }
    void set_noise_temperature(double val) { noise_temperature = val; }
};

#endif // FORMAN_RICCI_H
