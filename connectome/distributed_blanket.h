#ifndef DISTRIBUTED_BLANKET_H
#define DISTRIBUTED_BLANKET_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/templates/local_vector.h"
#include "connectome_graph.h"

struct NeuronState {
    // Internal states (μ)
    double membrane_potential = -65.0;
    double firing_rate = 0.0;
    double energy_reserve = 1.0;
    double prediction_error = 0.0;

    // Metabolic state (Szilard engine)
    double negentropy = 100.0;
    double waste_heat = 0.0;
    double temperature = 300.0;
    double efficiency = 0.99;

    // Belief distribution (normalized over cell types)
    double belief_entropy = 0.0;

    // Sensory input (accumulated from pre-synaptic partners)
    double sensory_sum = 0.0;
    double sensory_count = 0.0;

    // Active output (sent to post-synaptic partners)
    double active_output = 0.0;
};

class DistributedBlanket : public Resource {
    GDCLASS(DistributedBlanket, Resource);

protected:
    static void _bind_methods();

public:
    DistributedBlanket();

    Ref<ConnectomeGraph> graph;

    // Per-neuron state arrays (compact, index = vertex idx)
    Vector<NeuronState> states;

    // Global statistics
    double total_vfe = 0.0;
    double total_negentropy = 0.0;
    double total_waste_heat = 0.0;
    double mean_firing_rate = 0.0;
    double mean_prediction_error = 0.0;
    int active_neurons = 0;

    void set_graph(Ref<ConnectomeGraph> p_graph);
    Ref<ConnectomeGraph> get_graph() const { return graph; }

    void initialize();
    void process_step(double dt);
    void reset();

    // Per-neuron access
    double get_membrane_potential(int idx) const;
    double get_firing_rate(int idx) const;
    double get_negentropy(int idx) const;
    double get_prediction_error(int idx) const;

    // Global queries
    double get_total_vfe() const { return total_vfe; }
    double get_total_negentropy() const { return total_negentropy; }
    double get_mean_firing_rate() const { return mean_firing_rate; }
    int get_active_neuron_count() const { return active_neurons; }
    int get_neuron_count() const { return states.size(); }

private:
    void compute_sensory_input(int idx);
    void compute_active_output(int idx);
    void update_metabolism(int idx, double dt);
    void update_beliefs(int idx, double dt);
    double sigmoid(double x) const;
};

#endif // DISTRIBUTED_BLANKET_H
