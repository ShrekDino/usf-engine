#ifndef MARKOV_BLANKET_H
#define MARKOV_BLANKET_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"

class MarkovBlanket : public Resource {
    GDCLASS(MarkovBlanket, Resource);

protected:
    static void _bind_methods();

public:
    Vector<int> internal_indices;
    Vector<int> sensory_indices;
    Vector<int> active_indices;
    Vector<int> external_indices;

    Vector<double> internal_states;
    Vector<double> sensory_states;
    Vector<double> active_states;
    Vector<double> external_states;

    MarkovBlanket();

    void initialize(int n_internal, int n_sensory, int n_active, int n_external);

    bool is_permeable() const;
    double mutual_information() const;

    void sense(Vector<double> external_signals);
    Vector<double> act();
    void update_internal(double learning_rate);

    double entropy() const;
    double vitality() const;

    void seal();
    void unshield(Vector<double> external_signals);

    int get_n_internal() const { return internal_states.size(); }
    int get_n_sensory() const { return sensory_states.size(); }
    int get_n_active() const { return active_states.size(); }
    int get_n_external() const { return external_states.size(); }
};

#endif // MARKOV_BLANKET_H
