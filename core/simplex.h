#ifndef SIMPLEX_H
#define SIMPLEX_H

#include "core/object/class_db.h"
#include "core/templates/vector.h"

class Simplex {
public:
    enum Dimension {
        VERTEX = 0,
        EDGE = 1,
        TRIANGLE = 2,
        TETRAHEDRON = 3,
        FOUR_SIMPLEX = 4
    };

    Dimension dim;
    Vector<int> verts;

    Simplex();
    Simplex(Dimension p_dim, const Vector<int> &p_verts);

    static Simplex vertex(int v);
    static Simplex edge(int a, int b);
    static Simplex triangle(int a, int b, int c);
    static Simplex tetrahedron(int a, int b, int c, int d);
    static Simplex four_simplex(int a, int b, int c, int d, int e);

    int dimension() const;
    int size() const;
    int vertex(int idx) const;
    Vector<int> vertices() const;

    Vector<Simplex> boundary() const;

    bool contains_vertex(int vid) const;
    bool shares_face_with(const Simplex &other) const;

    bool operator==(const Simplex &other) const;
    bool operator!=(const Simplex &other) const;
    bool operator<(const Simplex &other) const;

    uint32_t hash() const;
    String to_string() const;

private:
    static void sort_verts(Vector<int> &v);
};

#endif // SIMPLEX_H
