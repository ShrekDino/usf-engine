#ifndef TORSION_FIELD_H
#define TORSION_FIELD_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "tetrad.h"
#include "../core/usf_constants.h"

class TorsionField : public Resource {
    GDCLASS(TorsionField, Resource);

protected:
    static void _bind_methods();

public:
    double spin_connection[4][4][4];
    double torsion_tensor[4][4][4];
    double contorsion[4][4][4];
    double torsion_scalar = 0.0;
    double torsion_energy_density = 0.0;
    double axial_vector[4];
    double axial_squared = 0.0;

    TorsionField();

    void compute(Ref<Tetrad> tetrad, Ref<USFConstants> constants);
    void clear();

    double get_torsion_scalar() const { return torsion_scalar; }
    double get_torsion_energy_density() const { return torsion_energy_density; }
    double get_axial_component(int mu) const;
    double get_axial_squared() const { return axial_squared; }

    double torsion_repulsion(double scale_factor) const;

private:
    double partial_derivative(const Ref<Tetrad> &tetrad, int a, int coord, int index, double dx) const;
    void extract_axial_vector(const Ref<Tetrad> &tetrad);
};

#endif // TORSION_FIELD_H
