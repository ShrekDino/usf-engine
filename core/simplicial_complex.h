#ifndef SIMPLICIAL_COMPLEX_H
#define SIMPLICIAL_COMPLEX_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/templates/local_vector.h"
#include "core/simplex.h"
#include "core/usf_constants.h"

struct Vec4d {
    double x, y, z, w;

    Vec4d() : x(0.0), y(0.0), z(0.0), w(0.0) {}
    Vec4d(double p_x, double p_y, double p_z, double p_w) : x(p_x), y(p_y), z(p_z), w(p_w) {}

    double operator[](int idx) const {
        switch (idx) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
            default: return 0.0;
        }
    }

    double &operator[](int idx) {
        switch (idx) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
            default: return x;
        }
    }
};

class SimplicialComplex : public Resource {
    GDCLASS(SimplicialComplex, Resource);

protected:
    static void _bind_methods();

public:
    Vector<Vec4d> vertices;
    Vector<Simplex> edges;
    Vector<Simplex> triangles;
    Vector<Simplex> tetrahedra;
    Vector<Simplex> four_simplices;
    Vector<double> edge_lengths;

    Ref<USFConstants> constants;
    int grid_n = 0;
    double spacing = 0.0;

    SimplicialComplex();

    void from_hypercubic_lattice(int n, double p_spacing, Ref<USFConstants> p_constants);

    static double minkowski_dot(const Vec4d &a, const Vec4d &b);
    Vec4d edge_vector(int i, int j) const;
    double edge_length_sq(int i, int j) const;
    double edge_length_raw(int i, int j) const;

    void compute_edge_lengths();

    int vertex_count() const { return vertices.size(); }
    int edge_count() const { return edges.size(); }
    int triangle_count() const { return triangles.size(); }
    int tetrahedron_count() const { return tetrahedra.size(); }
    int four_simplex_count() const { return four_simplices.size(); }

    const Vec4d &get_vertex(int i) const { return vertices[i]; }
    const Simplex &get_edge(int i) const { return edges[i]; }
    const Simplex &get_triangle(int i) const { return triangles[i]; }
    const Simplex &get_tetrahedron(int i) const { return tetrahedra[i]; }
    const Simplex &get_four_simplex(int i) const { return four_simplices[i]; }
    double get_edge_length(int i) const { return edge_lengths[i]; }

private:
    int idx(int i, int j, int k, int l) const;
    void triangulate_hypercubic(int n);
};

#endif // SIMPLICIAL_COMPLEX_H
