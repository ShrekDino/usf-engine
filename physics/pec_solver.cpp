#include "pec_solver.h"
#include "core/math/math_funcs.h"

PECSolver::PECSolver() :
        convergence(0.0),
        iteration(0) {
    constants.instantiate();
    regge.instantiate();
}

void PECSolver::set_complex(Ref<SimplicialComplex> p_complex) {
    complex = p_complex;
    if (complex.is_valid()) {
        int n = complex->four_simplex_count();
        n = n > 0 ? n : 1;
        tetrads.resize(n);
        torsion_fields.resize(n);
        for (int i = 0; i < n; i++) {
            tetrads.set(i, Tetrad::minkowski());
            torsion_fields.set(i, Ref<TorsionField>(memnew(TorsionField)));
        }
        regge->compute(complex);
    }
}

void PECSolver::set_constants(Ref<USFConstants> p_constants) {
    constants = p_constants;
}

double PECSolver::solve_step() {
    if (complex.is_null()) {
        return 0.0;
    }

    regge->compute(complex);

    for (int i = 0; i < torsion_fields.size() && i < tetrads.size(); i++) {
        torsion_fields[i]->compute(tetrads[i], constants);
    }

    double regge_residual = regge->get_regge_action();
    if (regge_residual < 0.0) {
        regge_residual = -regge_residual;
    }

    double total_torsion = 0.0;
    for (int i = 0; i < torsion_fields.size(); i++) {
        total_torsion += torsion_fields[i]->get_torsion_energy_density();
    }
    double torsion_residual = total_torsion / (double)torsion_fields.size();

    convergence = regge_residual + torsion_residual;
    iteration++;

    double lr = 1.0 / (1.0 + (double)iteration * 0.01);
    for (int i = 0; i < tetrads.size(); i++) {
        Ref<Tetrad> tetrad = tetrads[i];
        double grad = convergence * 0.01;
        for (int a = 0; a < 4; a++) {
            for (int mu = 0; mu < 4; mu++) {
                double val = tetrad->frame[a][mu];
                val -= lr * grad * val;
                tetrad->frame[a][mu] = val;
            }
        }
        tetrad->compute_coframe();
    }

    return convergence;
}

bool PECSolver::solve(int max_iterations, double tolerance) {
    for (int i = 0; i < max_iterations; i++) {
        double conv = solve_step();
        if (conv < tolerance) {
            return true;
        }
    }
    return false;
}

double PECSolver::einstein_tensor_component(int mu, int nu) const {
    if (regge.is_null() || regge->hinge_count() == 0) {
        return 0.0;
    }

    double avg_deficit = 0.0;
    for (int i = 0; i < regge->hinge_count(); i++) {
        avg_deficit += regge->deficit_angles[i];
    }
    avg_deficit /= (double)regge->hinge_count();

    double avg_volume = regge->total_hinge_volume() / (double)regge->hinge_count();

    if (mu == nu) {
        return avg_deficit / avg_volume;
    }
    return 0.0;
}

double PECSolver::stress_energy_tensor_component(int mu, int nu) const {
    if (torsion_fields.size() == 0) {
        return 0.0;
    }

    double avg_torsion = 0.0;
    for (int i = 0; i < torsion_fields.size(); i++) {
        avg_torsion += torsion_fields[i]->get_torsion_energy_density();
    }
    avg_torsion /= (double)torsion_fields.size();

    if (mu == nu) {
        return avg_torsion;
    }
    return 0.0;
}

void PECSolver::reset() {
    convergence = 0.0;
    iteration = 0;
    if (complex.is_valid()) {
        int n = complex->four_simplex_count();
        n = n > 0 ? n : 1;
        tetrads.resize(n);
        torsion_fields.resize(n);
        for (int i = 0; i < n; i++) {
            tetrads.set(i, Tetrad::minkowski());
            torsion_fields.set(i, Ref<TorsionField>(memnew(TorsionField)));
        }
        regge->compute(complex);
    }
}

void PECSolver::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_complex", "complex"), &PECSolver::set_complex);
    ClassDB::bind_method(D_METHOD("get_complex"), &PECSolver::get_complex);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "complex", PROPERTY_HINT_RESOURCE_TYPE, "SimplicialComplex"), "set_complex", "get_complex");

    ClassDB::bind_method(D_METHOD("set_constants", "constants"), &PECSolver::set_constants);
    ClassDB::bind_method(D_METHOD("get_constants"), &PECSolver::get_constants);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "constants", PROPERTY_HINT_RESOURCE_TYPE, "USFConstants"), "set_constants", "get_constants");

    ClassDB::bind_method(D_METHOD("solve_step"), &PECSolver::solve_step);
    ClassDB::bind_method(D_METHOD("solve", "max_iterations", "tolerance"), &PECSolver::solve);
    ClassDB::bind_method(D_METHOD("reset"), &PECSolver::reset);

    ClassDB::bind_method(D_METHOD("einstein_tensor_component", "mu", "nu"), &PECSolver::einstein_tensor_component);
    ClassDB::bind_method(D_METHOD("stress_energy_tensor_component", "mu", "nu"), &PECSolver::stress_energy_tensor_component);

    ClassDB::bind_method(D_METHOD("get_convergence"), &PECSolver::get_convergence);
    ClassDB::bind_method(D_METHOD("get_iteration"), &PECSolver::get_iteration);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "convergence", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_convergence");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "iteration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "", "get_iteration");
}
