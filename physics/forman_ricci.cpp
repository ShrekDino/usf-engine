#include "forman_ricci.h"
#include "core/math/math_funcs.h"
#include <cmath>

FormanRicci::FormanRicci() {
    constants.instantiate();
    planck_floor = constants->planck_length;
}

void FormanRicci::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
    planck_floor = constants->planck_length;
}

double FormanRicci::edge_weight(double edge_length) const {
    double l = std::abs(edge_length);
    if (l < planck_floor) {
        l = planck_floor;
    }
    return 1.0 / (l * l);
}

double FormanRicci::face_weight(double edge_a, double edge_b, double edge_c) const {
    double s1 = std::abs(edge_a);
    double s2 = std::abs(edge_b);
    double s3 = std::abs(edge_c);

    if (s1 < planck_floor) s1 = planck_floor;
    if (s2 < planck_floor) s2 = planck_floor;
    if (s3 < planck_floor) s3 = planck_floor;

    double sp = (s1 + s2 + s3) / 2.0;
    double area_sq = sp * (sp - s1) * (sp - s2) * (sp - s3);
    if (area_sq <= 0.0) {
        return 1.0;
    }
    double area = std::sqrt(area_sq);
    if (area < planck_floor * planck_floor) {
        area = planck_floor * planck_floor;
    }
    return 1.0 / area;
}

Vector<double> FormanRicci::compute_edge_curvatures(Ref<SimplicialComplex> complex) const {
    Vector<double> curvatures;
    int n_edges = complex->edge_count();
    curvatures.resize(n_edges);

    for (int i = 0; i < n_edges; i++) {
        curvatures.set(i, compute_ricci_for_edge(*complex.ptr(), i));
    }

    return curvatures;
}

double FormanRicci::compute_ricci_for_edge(const SimplicialComplex &complex, int edge_idx) const {
    const Simplex &e = complex.edges[edge_idx];
    int v0 = e.vertex(0);
    int v1 = e.vertex(1);

    return compute_ricci_from_vertices(complex, v0, v1);
}

double FormanRicci::compute_ricci_from_vertices(const SimplicialComplex &complex, int v0, int v1) const {
    double len = complex.edge_length_raw(v0, v1);
    double w_e = edge_weight(len);

    if (!std::isfinite(w_e) || w_e <= 0.0) {
        return 0.0;
    }

    double face_sum = 0.0;

    for (int ti = 0; ti < complex.triangle_count(); ti++) {
        const Simplex &tri = complex.triangles[ti];
        if (!tri.contains_vertex(v0) || !tri.contains_vertex(v1)) {
            continue;
        }

        int v_other = -1;
        for (int vi = 0; vi < 3; vi++) {
            int v = tri.vertex(vi);
            if (v != v0 && v != v1) {
                v_other = v;
                break;
            }
        }

        if (v_other >= 0) {
            double l0 = complex.edge_length_raw(v0, v_other);
            double l1 = complex.edge_length_raw(v1, v_other);
            double w_f = face_weight(len, l0, l1);
            if (std::isfinite(w_f)) {
                face_sum += w_f / w_e;
            }
        }
    }

    double vertex_sum = 2.0 / w_e;

    double parallel_sum = 0.0;

    for (int ei = 0; ei < complex.edge_count(); ei++) {
        const Simplex &other = complex.edges[ei];
        int ov0 = other.vertex(0);
        int ov1 = other.vertex(1);

        if (ov0 == v0 && ov1 == v1) continue;
        if (ov0 == v0 || ov0 == v1 || ov1 == v0 || ov1 == v1) {
            double other_len = complex.edge_length_raw(ov0, ov1);
            double w_other = edge_weight(other_len);
            if (std::isfinite(w_other)) {
                parallel_sum += std::abs(w_e - w_other) / w_e;
            }
        }
    }

    double ricci = w_e * (face_sum + vertex_sum - parallel_sum);
    if (!std::isfinite(ricci)) {
        return 0.0;
    }
    if (std::abs(ricci) > 1e50) {
        ricci = (ricci > 0.0) ? 1e50 : -1e50;
    }
    return ricci;
}

void FormanRicci::damp_metric(Ref<SimplicialComplex> complex, double dt) {
    double lw = planck_floor;

    for (int ei = 0; ei < complex->edge_count(); ei++) {
        double ricci = compute_ricci_for_edge(*complex.ptr(), ei);
        double current_len = complex->edge_lengths[ei];
        double abs_len = std::abs(current_len);

        double delta = -damping_coefficient * ricci * current_len * dt;

        if (noise_temperature > 0.0) {
            double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0;
            noise *= std::sqrt(2.0 * constants->k_b * noise_temperature * damping_coefficient * dt);
            delta += noise;
        }

        double new_len = current_len + delta;
        double new_abs = std::abs(new_len);
        if (new_abs < lw) {
            new_len = (new_len >= 0.0) ? lw : -lw;
        }

        complex->edge_lengths.set(ei, new_len);
    }

    complex->compute_edge_lengths();
}

double FormanRicci::topological_thermostat(double signal_to_noise) const {
    double snr = signal_to_noise;
    if (snr < 1e-30) {
        snr = 1e-30;
    }
    double ratio = I_0 / snr;
    return gamma_0 * std::pow(1.0 + ratio * ratio, thermostat_alpha * 0.5);
}

void FormanRicci::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &FormanRicci::set_constants);
    ClassDB::bind_method(D_METHOD("get_constants"), &FormanRicci::get_constants);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "constants", PROPERTY_HINT_RESOURCE_TYPE, "USFConstants"), "set_constants", "get_constants");

    ClassDB::bind_method(D_METHOD("set_damping_coefficient", "val"), &FormanRicci::set_damping_coefficient);
    ClassDB::bind_method(D_METHOD("get_damping_coefficient"), &FormanRicci::get_damping_coefficient);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "damping_coefficient", PROPERTY_HINT_RANGE, "0.0,100.0,0.01"), "set_damping_coefficient", "get_damping_coefficient");

    ClassDB::bind_method(D_METHOD("set_noise_temperature", "val"), &FormanRicci::set_noise_temperature);
    ClassDB::bind_method(D_METHOD("get_noise_temperature"), &FormanRicci::get_noise_temperature);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_temperature", PROPERTY_HINT_RANGE, "0.0,1e10,1.0"), "set_noise_temperature", "get_noise_temperature");

    ClassDB::bind_method(D_METHOD("compute_edge_curvatures", "complex"), &FormanRicci::compute_edge_curvatures);
    ClassDB::bind_method(D_METHOD("damp_metric", "complex", "dt"), &FormanRicci::damp_metric);
    ClassDB::bind_method(D_METHOD("topological_thermostat", "signal_to_noise"), &FormanRicci::topological_thermostat);
}
