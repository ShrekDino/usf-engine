#ifndef VITALITY_MONITOR_H
#define VITALITY_MONITOR_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "connectome_graph.h"
#include "distributed_blanket.h"
#include "global_workspace.h"

class VitalityMonitor : public Resource {
    GDCLASS(VitalityMonitor, Resource);

protected:
    static void _bind_methods();

public:
    VitalityMonitor();

    Ref<ConnectomeGraph> graph;
    Ref<DistributedBlanket> blanket;
    Ref<GlobalWorkspace> workspace;

    // Senescence tracking
    Vector<double> neuron_efficiency;
    Vector<int> senescence_counter;
    Vector<bool> senescent;
    int senescent_count = 0;
    double senescence_threshold = 0.1;
    int senescence_max_steps = 50;

    // V_network
    double v_network = 0.0;
    double v_network_history[100];
    int vn_history_idx = 0;
    double mutual_info_sum = 0.0;

    // Curiosity engine
    Vector<double> vfe_history;
    double vfe_variance = 0.0;
    double curiosity_drive = 0.0;
    double curiosity_threshold = 0.01;
    int curiosity_cooldown = 0;
    bool exploring = false;

    // HALT prevention
    int halt_risk_neurons = 0;
    int halt_prevention_triggers = 0;

    void set_graph(Ref<ConnectomeGraph> p_graph);
    void set_blanket(Ref<DistributedBlanket> p_blanket);
    void set_workspace(Ref<GlobalWorkspace> p_workspace);

    void initialize();
    void process_step(double dt);
    void reset();

    // Senescence queries
    double get_senescent_count() const { return senescent_count; }
    int get_halt_risk_neurons() const { return halt_risk_neurons; }
    double get_neuron_efficiency(int idx) const;

    // V_network
    double get_v_network() const { return v_network; }
    double get_mutual_info_sum() const { return mutual_info_sum; }

    // Curiosity
    double get_curiosity_drive() const { return curiosity_drive; }
    bool is_exploring() const { return exploring; }

    // HALT
    int get_halt_prevention_triggers() const { return halt_prevention_triggers; }
    double get_vfe_variance() const { return vfe_variance; }

private:
    void compute_senescence();
    void compute_v_network(double dt);
    void compute_curiosity(double dt);
    void compute_halt_prevention();
};

#endif // VITALITY_MONITOR_H
