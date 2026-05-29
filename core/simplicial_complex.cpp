#include "simplicial_complex.h"
#include "core/math/math_funcs.h"
#include <set>

SimplicialComplex::SimplicialComplex() {
    constants.instantiate();
}

void SimplicialComplex::from_hypercubic_lattice(int n, double p_spacing, Ref<USFConstants> p_constants) {
    constants = p_constants;
    grid_n = n;

    double lw = constants->planck_length;
    double clamped_spacing = p_spacing > lw ? p_spacing : lw;
    spacing = clamped_spacing;

    int total = n * n * n * n;
    vertices.resize(total);

    int vi = 0;
    for (int i = 0; i < n; i++) {
        double t = (double)i * clamped_spacing;
        for (int j = 0; j < n; j++) {
            double x = (double)j * clamped_spacing;
            for (int k = 0; k < n; k++) {
                double y = (double)k * clamped_spacing;
                for (int l = 0; l < n; l++) {
                    double z = (double)l * clamped_spacing;
                    vertices.set(vi++, Vec4d(t, x, y, z));
                }
            }
        }
    }

    triangulate_hypercubic(n);
    compute_edge_lengths();
}

double SimplicialComplex::minkowski_dot(const Vec4d &a, const Vec4d &b) {
    return -(a.x * b.x) + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vec4d SimplicialComplex::edge_vector(int i, int j) const {
    const Vec4d &vi = vertices[i];
    const Vec4d &vj = vertices[j];
    return Vec4d(
            vj.x - vi.x,
            vj.y - vi.y,
            vj.z - vi.z,
            vj.w - vi.w);
}

double SimplicialComplex::edge_length_sq(int i, int j) const {
    Vec4d dx = edge_vector(i, j);
    return minkowski_dot(dx, dx);
}

double SimplicialComplex::edge_length_raw(int i, int j) const {
    double ds2 = edge_length_sq(i, j);
    if (ds2 < 0.0) {
        return -Math::sqrt(-ds2);
    }
    return Math::sqrt(ds2);
}

void SimplicialComplex::compute_edge_lengths() {
    double lw = constants->planck_length;
    edge_lengths.resize(edges.size());

    for (int ei = 0; ei < edges.size(); ei++) {
        const Simplex &e = edges[ei];
        int v0 = e.vertex(0);
        int v1 = e.vertex(1);
        double raw = edge_length_raw(v0, v1);
        double abs_raw = raw >= 0.0 ? raw : -raw;
        if (abs_raw < lw) {
            edge_lengths.set(ei, raw >= 0.0 ? lw : -lw);
        } else {
            edge_lengths.set(ei, raw);
        }
    }
}

int SimplicialComplex::idx(int i, int j, int k, int l) const {
    return i * grid_n * grid_n * grid_n + j * grid_n * grid_n + k * grid_n + l;
}

void SimplicialComplex::triangulate_hypercubic(int n) {
    std::set<Simplex> edge_set;
    std::set<Simplex> tri_set;
    std::set<Simplex> tet_set;

    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1; j++) {
            for (int k = 0; k < n - 1; k++) {
                for (int l = 0; l < n - 1; l++) {
                    int v = idx(i, j, k, l);
                    int vx = idx(i + 1, j, k, l);
                    int vy = idx(i, j + 1, k, l);
                    int vz = idx(i, j, k + 1, l);
                    int vt = idx(i, j, k, l + 1);

                    int vxy = idx(i + 1, j + 1, k, l);
                    int vxz = idx(i + 1, j, k + 1, l);
                    int vxt = idx(i + 1, j, k, l + 1);
                    int vyz = idx(i, j + 1, k + 1, l);
                    int vyt = idx(i, j + 1, k, l + 1);
                    int vzt = idx(i, j, k + 1, l + 1);

                    int vxyz = idx(i + 1, j + 1, k + 1, l);
                    int vxyt = idx(i + 1, j + 1, k, l + 1);
                    int vxzt = idx(i + 1, j, k + 1, l + 1);
                    int vyzt = idx(i, j + 1, k + 1, l + 1);
                    int vxyzt = idx(i + 1, j + 1, k + 1, l + 1);

                    int verts[16] = {
                        v, vx, vy, vz, vt, vxy, vxz, vxt,
                        vyz, vyt, vzt, vxyz, vxyt, vxzt, vyzt, vxyzt
                    };

                    for (int a = 0; a < 16; a++) {
                        for (int b = a + 1; b < 16; b++) {
                            edge_set.insert(Simplex::make_edge(verts[a], verts[b]));
                        }
                    }

                    for (int a = 0; a < 16; a++) {
                        for (int b = a + 1; b < 16; b++) {
                            for (int c = b + 1; c < 16; c++) {
                                tri_set.insert(Simplex::make_triangle(verts[a], verts[b], verts[c]));
                            }
                        }
                    }

                    for (int a = 0; a < 16; a++) {
                        for (int b = a + 1; b < 16; b++) {
                            for (int c = b + 1; c < 16; c++) {
                                for (int d = c + 1; d < 16; d++) {
                                    tet_set.insert(Simplex::make_tetrahedron(verts[a], verts[b], verts[c], verts[d]));
                                }
                            }
                        }
                    }

                    four_simplices.push_back(Simplex::make_four_simplex(v, vx, vxy, vxyz, vxyzt));
                    four_simplices.push_back(Simplex::make_four_simplex(v, vy, vxy, vxyz, vxyzt));
                    four_simplices.push_back(Simplex::make_four_simplex(v, vx, vxz, vxyz, vxyzt));
                    four_simplices.push_back(Simplex::make_four_simplex(v, vz, vxz, vxyz, vxyzt));
                    four_simplices.push_back(Simplex::make_four_simplex(v, vy, vyz, vxyz, vxyzt));
                    four_simplices.push_back(Simplex::make_four_simplex(v, vz, vyz, vxyz, vxyzt));
                }
            }
        }
    }

    for (const Simplex &e : edge_set) {
        edges.push_back(e);
    }
    for (const Simplex &t : tri_set) {
        triangles.push_back(t);
    }
    for (const Simplex &t : tet_set) {
        tetrahedra.push_back(t);
    }
}

void SimplicialComplex::_bind_methods() {
    ClassDB::bind_method(D_METHOD("from_hypercubic_lattice", "n", "spacing", "constants"), &SimplicialComplex::from_hypercubic_lattice);

    ClassDB::bind_method(D_METHOD("get_vertex_count"), &SimplicialComplex::vertex_count);
    ClassDB::bind_method(D_METHOD("get_edge_count"), &SimplicialComplex::edge_count);
    ClassDB::bind_method(D_METHOD("get_triangle_count"), &SimplicialComplex::triangle_count);
    ClassDB::bind_method(D_METHOD("get_tetrahedron_count"), &SimplicialComplex::tetrahedron_count);
    ClassDB::bind_method(D_METHOD("get_four_simplex_count"), &SimplicialComplex::four_simplex_count);

    ClassDB::bind_method(D_METHOD("get_edge_length", "index"), &SimplicialComplex::get_edge_length);
}
