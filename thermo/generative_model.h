#ifndef GENERATIVE_MODEL_H
#define GENERATIVE_MODEL_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/usf_constants.h"

class GenerativeModel : public Resource {
    GDCLASS(GenerativeModel, Resource);

protected:
    static void _bind_methods();

public:
    Vector<double> beliefs;
    Vector<Vector<double>> likelihood;
    Vector<double> prior;
    int n_states = 0;
    int n_observations = 0;

    Ref<USFConstants> constants;

    GenerativeModel();

    void set_constants(Ref<USFConstants> p_constants);
    Ref<USFConstants> get_constants() const { return constants; }

    void initialize(int p_n_states, int p_n_observations);
    void from_params(Vector<double> p_beliefs, Vector<Vector<double>> p_likelihood, Vector<double> p_prior);

    double variational_free_energy(Vector<double> observation) const;
    void update_beliefs(Vector<double> observation, double learning_rate);

    double predictive_accuracy() const;
    double entropy() const;
    double metabolic_cost() const;

    int get_n_states() const { return n_states; }
    int get_n_observations() const { return n_observations; }
    double get_belief(int i) const;
    void set_belief(int i, double val);
};

#endif // GENERATIVE_MODEL_H
