#include "core/math/math_defs.h"
#include "regge_tensor.h"
#include "core/math/math_funcs.h"
#include <cmath>

ReggeTensor::ReggeTensor() :
        regge_action(0.0) {}

void ReggeTensor::compute(Ref<SimplicialComplex> complex) {
    int n_tri = complex->triangle_count();
    deficit_angles.resize(n_tri);
    hinge_volumes.resize(n_tri);

    for (int ti = 0; ti < n_tri; ti++) {
        const Simplex &tri = complex->triangles[ti];
        int a = tri.vertex(0);
        int b = tri.vertex(1);
        int c = tri.vertex(2);

        Vec4d ab = complex->edge_vector(a, b);
        Vec4d ac = complex->edge_vector(a, c);
        Vec4d bc = complex->edge_vector(b, c);

        double ab_sq = SimplicialComplex::minkowski_dot(ab, ab);
        double ac_sq = SimplicialComplex::minkowski_dot(ac, ac);
        double bc_sq = SimplicialComplex::minkowski_dot(bc, bc);

        double area = triangle_area_4d(ab_sq, ac_sq, bc_sq);
        hinge_volumes.set(ti, area);

        double total_dihedral = 0.0;
        int tetra_count = 0;

        for (int tei = 0; tei < complex->tetrahedron_count(); tei++) {
            const Simplex &tet = complex->tetrahedra[tei];
            if (!tet.contains_vertex(a) || !tet.contains_vertex(b)) {
                continue;
            }

            int d = -1;
            for (int vi = 0; vi < 4; vi++) {
                int v = tet.vertex(vi);
                if (v != a && v != b && v != c) {
                    d = v;
                    break;
                }
            }

            if (d == -1 && tet.contains_vertex(c)) {
                int found = 0;
                for (int vi = 0; vi < 4; vi++) {
                    int v = tet.vertex(vi);
                    if (v == a || v == b || v == c) {
                        found++;
                    }
                }
                if (found == 3) {
                    for (int vi = 0; vi < 4; vi++) {
                        int v = tet.vertex(vi);
                        if (v != a && v != b && v != c) {
                            d = v;
                            break;
                        }
                    }
                }
            }

            if (d >= 0) {
                double theta = dihedral_angle_4d(*complex.ptr(), a, b, c, d);
                total_dihedral += theta;
                tetra_count++;
            }
        }

        double deficit = 0.0;
        if (tetra_count > 0) {
            deficit = 2.0 * Math::PI - total_dihedral;
            if (deficit < 0.0) {
                deficit = 0.0;
            }
        }

        deficit_angles.set(ti, deficit);
    }

    regge_action = 0.0;
    double prefactor = 1.0 / (16.0 * Math::PI * complex->constants->g);
    for (int i = 0; i < n_tri; i++) {
        regge_action += prefactor * hinge_volumes[i] * deficit_angles[i];
    }

    edge_lengths_sq.resize(complex->edge_count());
    for (int i = 0; i < complex->edge_count(); i++) {
        double l = complex->edge_lengths[i];
        edge_lengths_sq.set(i, l * l);
    }
}

double ReggeTensor::triangle_area_4d(double ab_sq, double ac_sq, double bc_sq) {
    double s1 = std::sqrt(std::abs(ab_sq));
    double s2 = std::sqrt(std::abs(ac_sq));
    double s3 = std::sqrt(std::abs(bc_sq));

    if (s1 <= 0.0 || s2 <= 0.0 || s3 <= 0.0) {
        return 0.0;
    }

    double sp = (s1 + s2 + s3) / 2.0;
    double area_sq = sp * (sp - s1) * (sp - s2) * (sp - s3);
    if (area_sq > 0.0) {
        return std::sqrt(area_sq);
    }
    return 0.0;
}

double ReggeTensor::dihedral_angle_4d(const SimplicialComplex &complex, int a, int b, int c, int d) {
    Vec4d ab = complex.edge_vector(a, b);
    Vec4d ac = complex.edge_vector(a, c);
    Vec4d ad = complex.edge_vector(a, d);

    double ab_ac = SimplicialComplex::minkowski_dot(ab, ac);
    double ab_ad = SimplicialComplex::minkowski_dot(ab, ad);
    double ac_ad = SimplicialComplex::minkowski_dot(ac, ad);
    double ab_sq = SimplicialComplex::minkowski_dot(ab, ab);
    double ac_sq = SimplicialComplex::minkowski_dot(ac, ac);
    double ad_sq = SimplicialComplex::minkowski_dot(ad, ad);

    double ab_perp_ac = ac_sq - ab_ac * ab_ac / (ab_sq);
    double ab_perp_ad = ad_sq - ab_ad * ab_ad / (ab_sq);
    double cross = ac_ad - ab_ac * ab_ad / (ab_sq);

    if (ab_perp_ac <= 0.0 || ab_perp_ad <= 0.0) {
        return Math::PI / 3.0;
    }

    double cos_theta = cross / (std::sqrt(ab_perp_ac) * std::sqrt(ab_perp_ad));
    if (cos_theta > 1.0) {
        cos_theta = 1.0;
    }
    if (cos_theta < -1.0) {
        cos_theta = -1.0;
    }

    return std::acos(cos_theta);
}

double ReggeTensor::tetrahedron_volume_3d(const SimplicialComplex &complex, int a, int b, int c, int d) {
    Vec4d ab = complex.edge_vector(a, b);
    Vec4d ac = complex.edge_vector(a, c);
    Vec4d ad = complex.edge_vector(a, d);

    double det = ab.x * (ac.y * ad.z - ac.z * ad.y)
                 - ab.y * (ac.x * ad.z - ac.z * ad.x)
                 + ab.z * (ac.x * ad.y - ac.y * ad.x);

    return std::abs(det) / 6.0;
}

double ReggeTensor::simplex_4d_volume(const SimplicialComplex &complex, const Vector<int> &verts) {
    if (verts.size() < 5) {
        return 0.0;
    }

    Vec4d v0 = complex.vertices[verts[0]];
    Vec4d e1 = complex.vertices[verts[1]];
    Vec4d e2 = complex.vertices[verts[2]];
    Vec4d e3 = complex.vertices[verts[3]];
    Vec4d e4 = complex.vertices[verts[4]];

    Vec4d d1(e1.x - v0.x, e1.y - v0.y, e1.z - v0.z, e1.w - v0.w);
    Vec4d d2(e2.x - v0.x, e2.y - v0.y, e2.z - v0.z, e2.w - v0.w);
    Vec4d d3(e3.x - v0.x, e3.y - v0.y, e3.z - v0.z, e3.w - v0.w);
    Vec4d d4(e4.x - v0.x, e4.y - v0.y, e4.z - v0.z, e4.w - v0.w);

    double det = 0.0;
    for (int i = 0; i < 4; i++) {
        double sub = 0.0;
        int i1 = (i + 1) % 4;
        int i2 = (i + 2) % 4;
        int i3 = (i + 3) % 4;

        double m1 = d1[i1] * (d2[i2] * d3[i3] - d2[i3] * d3[i2])
                    - d1[i2] * (d2[i1] * d3[i3] - d2[i3] * d3[i1])
                    + d1[i3] * (d2[i1] * d3[i2] - d2[i2] * d3[i1]);

        sub = (i % 2 == 0) ? m1 : -m1;
        det += d4[i] * sub;
    }

    return std::abs(det) / 24.0;
}

double ReggeTensor::scalar_curvature_at(int hinge_idx) const {
    double eps = deficit_angles[hinge_idx];
    double v = hinge_volumes[hinge_idx];
    if (v > 0.0) {
        return 2.0 * eps / v;
    }
    return 0.0;
}

double ReggeTensor::average_curvature() const {
    double total_v = total_hinge_volume();
    if (total_v > 0.0) {
        return regge_action * 16.0 * Math::PI * 2.0 / total_v;
    }
    return 0.0;
}

double ReggeTensor::total_hinge_volume() const {
    double total = 0.0;
    for (int i = 0; i < hinge_volumes.size(); i++) {
        total += hinge_volumes[i];
    }
    return total;
}

void ReggeTensor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("compute", "complex"), &ReggeTensor::compute);

    ClassDB::bind_method(D_METHOD("get_hinge_count"), &ReggeTensor::hinge_count);
    ClassDB::bind_method(D_METHOD("get_deficit_angle", "index"), &ReggeTensor::get_deficit_angle);
    ClassDB::bind_method(D_METHOD("get_hinge_volume", "index"), &ReggeTensor::get_hinge_volume);
    ClassDB::bind_method(D_METHOD("get_regge_action"), &ReggeTensor::get_regge_action);

    ClassDB::bind_method(D_METHOD("scalar_curvature_at", "hinge_idx"), &ReggeTensor::scalar_curvature_at);
    ClassDB::bind_method(D_METHOD("average_curvature"), &ReggeTensor::average_curvature);
    ClassDB::bind_method(D_METHOD("total_hinge_volume"), &ReggeTensor::total_hinge_volume);
}
