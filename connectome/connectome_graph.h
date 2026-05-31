#ifndef CONNECTOME_GRAPH_H
#define CONNECTOME_GRAPH_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/templates/vector.h"
#include "core/templates/local_vector.h"
#include "core/string/ustring.h"

struct ConnectomeVertex {
    uint64_t root_id = 0;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    uint16_t cell_type_idx = 0;
    uint16_t side_idx = 0;
    uint16_t nt_type_idx = 0;
    uint16_t flow_idx = 0;
};

struct ConnectomeEdge {
    uint32_t pre_idx = 0;
    uint32_t post_idx = 0;
    float synapse_count = 0.0f;
    uint16_t neuropil_idx = 0;
    uint16_t nt_type_idx = 0;
};

struct ConnectomeTet {
    uint32_t v0 = 0, v1 = 0, v2 = 0, v3 = 0;
};

class ConnectomeGraph : public Resource {
    GDCLASS(ConnectomeGraph, Resource);

protected:
    static void _bind_methods();

public:
    ConnectomeGraph();

    // Vertex data
    Vector<ConnectomeVertex> vertices;
    Vector<uint32_t> vertex_ids;

    // Edge data
    Vector<ConnectomeEdge> edges;

    // Tetrahedra (from 3D Delaunay triangulation)
    Vector<ConnectomeTet> tetrahedra;

    // Adjacency: for each vertex idx, list of edge indices (outgoing = pre→post)
    Vector<Vector<uint32_t>> outgoing_edges;
    Vector<Vector<uint32_t>> incoming_edges;

    // Metadata string tables
    Vector<String> cell_type_table;
    Vector<String> side_table;
    Vector<String> nt_type_table;
    Vector<String> flow_table;
    Vector<String> neuropil_table;

    // Loading
    bool load_csv(const String &vertices_path, const String &edges_path, const String &somas_path);
    bool load_binary(const String &path);
    bool save_binary(const String &path) const;

    // Queries
    int get_vertex_count() const { return vertices.size(); }
    int get_edge_count() const { return edges.size(); }
    int get_tetrahedron_count() const { return tetrahedra.size(); }

    int find_vertex_by_root_id(uint64_t root_id) const;
    uint64_t get_vertex_root_id(int idx) const;

    float get_vertex_x(int idx) const;
    float get_vertex_y(int idx) const;
    float get_vertex_z(int idx) const;

    String get_cell_type(int idx) const;
    String get_side(int idx) const;
    String get_nt_type(int idx) const;
    String get_flow(int idx) const;
    String get_neuropil(int edge_idx) const;
    String get_neuropil_by_idx(int np_idx) const;
    int get_neuropil_count() const { return neuropil_table.size(); }
    Vector<int> get_vertices_by_neuropil(const String &name) const;
    String get_edge_nt_type(int edge_idx) const;

    float get_edge_synapse_count(int edge_idx) const;
    int get_edge_pre(int edge_idx) const;
    int get_edge_post(int edge_idx) const;

    // Graph traversal
    int get_outgoing_count(int vertex_idx) const;
    int get_incoming_count(int vertex_idx) const;
    int get_outgoing_edge(int vertex_idx, int i) const;
    int get_incoming_edge(int vertex_idx, int i) const;

    int get_max_outgoing() const;
    int get_max_incoming() const;
    double get_mean_outgoing() const;

    uint32_t get_tet_vertex(int tet_idx, int vertex_slot) const;
    bool load_with_tets(const String &path);

    void clear();

private:
    int resolve_string(Vector<String> &table, const String &value);
    uint32_t resolve_id(const Vector<uint32_t> &ids, uint64_t root_id) const;
};

#endif // CONNECTOME_GRAPH_H
