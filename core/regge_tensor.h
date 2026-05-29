#ifndef REGGE_TENSOR_H
#define REGGE_TENSOR_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "simplicial_complex.h"
#include "usf_constants.h"

class ReggeTensor : public Resource {
    GDCLASS(ReggeTensor, Resource);

protected:
    static void _bind_methods();

public:
    Vector<double> edge_lengths_sq;
    Vector<double> deficit_angles;
    Vector<double> hinge_volumes;
    double regge_action = 0.0;

    ReggeTensor();

    void compute(Ref<SimplicialComplex> complex);

    double scalar_curvature_at(int hinge_idx) const;
    double average_curvature() const;
    double total_hinge_volume() const;

    int hinge_count() const { return deficit_angles.size(); }
    double get_deficit_angle(int i) const { return deficit_angles[i]; }
    double get_hinge_volume(int i) const { return hinge_volumes[i]; }
    double get_regge_action() const { return regge_action; }

private:
    static double triangle_area_4d(double ab_sq, double ac_sq, double bc_sq);
    static double dihedral_angle_4d(const SimplicialComplex &complex, int a, int b, int c, int d);
    static double tetrahedron_volume_3d(const SimplicialComplex &complex, int a, int b, int c, int d);
    static double simplex_4d_volume(const SimplicialComplex &complex, const Vector<int> &verts);
};

#endif // REGGE_TENSOR_H
