#ifndef GLOBAL_WORKSPACE_H
#define GLOBAL_WORKSPACE_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "connectome_graph.h"
#include "distributed_blanket.h"

class GlobalWorkspace : public Resource {
    GDCLASS(GlobalWorkspace, Resource);

protected:
    static void _bind_methods();

public:
    GlobalWorkspace();

    Ref<ConnectomeGraph> graph;
    Ref<DistributedBlanket> blanket;

    // Rich-club hub indices (high-degree neurons)
    Vector<int> rich_club_indices;
    Vector<double> rich_club_degrees;
    int n_rich_club = 0;
    double rich_club_threshold = 0.0;

    // φ* metrics
    double phi_star = 0.0;
    double phi_star_history[100];
    int phi_history_idx = 0;

    // Global ignition
    double ignition_level = 0.0;
    bool ignited = false;
    double ignition_threshold = 0.5;
    int n_ignited_neurons = 0;

    // Dual-scalar field φ (phenomenological order parameter)
    double phi_field = 0.0;
    double phi_synchrony = 0.0;

    // Global workspace statistics
    double mean_rich_club_firing = 0.0;
    double mean_non_rich_club_firing = 0.0;
    double workspace_discrepancy = 0.0;

    void set_graph(Ref<ConnectomeGraph> p_graph);
    void set_blanket(Ref<DistributedBlanket> p_blanket);

    void initialize();
    void process_step(double dt);
    void reset();

    // Rich-club queries
    int get_rich_club_count() const { return n_rich_club; }
    double get_rich_club_threshold() const { return rich_club_threshold; }
    bool is_rich_club(int idx) const;

    // φ* queries
    double get_phi_star() const { return phi_star; }
    double get_phi_star_history(int i) const;

    // Ignition queries
    double get_ignition_level() const { return ignition_level; }
    bool is_ignited() const { return ignited; }
    int get_ignited_count() const { return n_ignited_neurons; }

    // φ field queries
    double get_phi_field() const { return phi_field; }
    double get_phi_synchrony() const { return phi_synchrony; }

private:
    void detect_rich_club();
    double compute_phi_star(int n_samples);
    double compute_synchrony();
    void compute_ignition();
    double mutual_information_between_sets(const Vector<int> &set_a, const Vector<int> &set_b);
};

#endif // GLOBAL_WORKSPACE_H
