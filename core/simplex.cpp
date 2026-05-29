#include "core/simplex.h"
#include "core/string/ustring.h"
#include "core/string/print_string.h"

Simplex::Simplex() :
        dim(VERTEX),
        verts() {}

Simplex::Simplex(Dimension p_dim, const Vector<int> &p_verts) :
        dim(p_dim),
        verts(p_verts) {
    sort_verts(verts);
}

Simplex Simplex::vertex(int v) {
    Vector<int> vv;
    vv.push_back(v);
    return Simplex(VERTEX, vv);
}

Simplex Simplex::edge(int a, int b) {
    Vector<int> vv;
    vv.push_back(a);
    vv.push_back(b);
    return Simplex(EDGE, vv);
}

Simplex Simplex::triangle(int a, int b, int c) {
    Vector<int> vv;
    vv.push_back(a);
    vv.push_back(b);
    vv.push_back(c);
    return Simplex(TRIANGLE, vv);
}

Simplex Simplex::tetrahedron(int a, int b, int c, int d) {
    Vector<int> vv;
    vv.push_back(a);
    vv.push_back(b);
    vv.push_back(c);
    vv.push_back(d);
    return Simplex(TETRAHEDRON, vv);
}

Simplex Simplex::four_simplex(int a, int b, int c, int d, int e) {
    Vector<int> vv;
    vv.push_back(a);
    vv.push_back(b);
    vv.push_back(c);
    vv.push_back(d);
    vv.push_back(e);
    return Simplex(FOUR_SIMPLEX, vv);
}

int Simplex::dimension() const {
    return (int)dim;
}

int Simplex::size() const {
    return verts.size();
}

int Simplex::vertex(int idx) const {
    return verts[idx];
}

Vector<int> Simplex::vertices() const {
    return verts;
}

Vector<Simplex> Simplex::boundary() const {
    Vector<Simplex> result;
    switch (dim) {
        case VERTEX:
            break;
        case EDGE: {
            result.push_back(Simplex::vertex(verts[0]));
            result.push_back(Simplex::vertex(verts[1]));
            break;
        }
        case TRIANGLE: {
            result.push_back(Simplex::edge(verts[1], verts[2]));
            result.push_back(Simplex::edge(verts[0], verts[2]));
            result.push_back(Simplex::edge(verts[0], verts[1]));
            break;
        }
        case TETRAHEDRON: {
            result.push_back(Simplex::triangle(verts[1], verts[2], verts[3]));
            result.push_back(Simplex::triangle(verts[0], verts[2], verts[3]));
            result.push_back(Simplex::triangle(verts[0], verts[1], verts[3]));
            result.push_back(Simplex::triangle(verts[0], verts[1], verts[2]));
            break;
        }
        case FOUR_SIMPLEX: {
            result.push_back(Simplex::tetrahedron(verts[1], verts[2], verts[3], verts[4]));
            result.push_back(Simplex::tetrahedron(verts[0], verts[2], verts[3], verts[4]));
            result.push_back(Simplex::tetrahedron(verts[0], verts[1], verts[3], verts[4]));
            result.push_back(Simplex::tetrahedron(verts[0], verts[1], verts[2], verts[4]));
            result.push_back(Simplex::tetrahedron(verts[0], verts[1], verts[2], verts[3]));
            break;
        }
    }
    return result;
}

bool Simplex::contains_vertex(int vid) const {
    for (int i = 0; i < verts.size(); i++) {
        if (verts[i] == vid)
            return true;
    }
    return false;
}

bool Simplex::shares_face_with(const Simplex &other) const {
    int count = 0;
    for (int i = 0; i < verts.size(); i++) {
        for (int j = 0; j < other.verts.size(); j++) {
            if (verts[i] == other.verts[j]) {
                count++;
                break;
            }
        }
    }
    int expected = (dimension() < other.dimension() ? dimension() : other.dimension()) + 1;
    return count == expected;
}

bool Simplex::operator==(const Simplex &other) const {
    if (dim != other.dim)
        return false;
    if (verts.size() != other.verts.size())
        return false;
    for (int i = 0; i < verts.size(); i++) {
        if (verts[i] != other.verts[i])
            return false;
    }
    return true;
}

bool Simplex::operator!=(const Simplex &other) const {
    return !(*this == other);
}

bool Simplex::operator<(const Simplex &other) const {
    if (dim != other.dim)
        return dim < other.dim;
    int n = verts.size();
    int m = other.verts.size();
    if (n != m)
        return n < m;
    for (int i = 0; i < n; i++) {
        if (verts[i] != other.verts[i])
            return verts[i] < other.verts[i];
    }
    return false;
}

uint32_t Simplex::hash() const {
    uint32_t h = (uint32_t)dim;
    for (int i = 0; i < verts.size(); i++) {
        h = h * 31 + (uint32_t)verts[i];
    }
    return h;
}

String Simplex::to_string() const {
    String s = "Simplex<" + itos(dimension()) + ">(";
    for (int i = 0; i < verts.size(); i++) {
        if (i > 0)
            s += ",";
        s += itos(verts[i]);
    }
    s += ")";
    return s;
}

void Simplex::sort_verts(Vector<int> &v) {
    int n = v.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (v[j] > v[j + 1]) {
                int tmp = v[j];
                v[j] = v[j + 1];
                v[j + 1] = tmp;
            }
        }
    }
}
