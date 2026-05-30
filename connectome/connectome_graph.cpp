#include "connectome_graph.h"
#include "core/io/file_access.h"
#include "core/math/math_funcs.h"
#include "core/string/ustring.h"
#include "core/string/print_string.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ConnectomeGraph::ConnectomeGraph() {}

uint32_t ConnectomeGraph::get_tet_vertex(int tet_idx, int vertex_slot) const {
    if (tet_idx < 0 || tet_idx >= tetrahedra.size() || vertex_slot < 0 || vertex_slot >= 4) return 0;
    switch (vertex_slot) {
        case 0: return tetrahedra[tet_idx].v0;
        case 1: return tetrahedra[tet_idx].v1;
        case 2: return tetrahedra[tet_idx].v2;
        case 3: return tetrahedra[tet_idx].v3;
        default: return 0;
    }
}

bool ConnectomeGraph::load_with_tets(const String &path) {
    bool ok = load_binary(path);
    if (ok && tetrahedra.size() == 0) {
        print_line("[Connectome] WARNING: No tetrahedra in binary (version 1 format). Regenerate with import_flywire.py --generate-tets");
    }
    return ok;
}

void ConnectomeGraph::clear() {
    vertices.clear();
    vertex_ids.clear();
    edges.clear();
    outgoing_edges.clear();
    incoming_edges.clear();
    tetrahedra.clear();
    cell_type_table.clear();
    side_table.clear();
    nt_type_table.clear();
    flow_table.clear();
    neuropil_table.clear();
}

int ConnectomeGraph::resolve_string(Vector<String> &table, const String &value) {
    for (int i = 0; i < table.size(); i++) {
        if (table[i] == value) {
            return i;
        }
    }
    table.push_back(value);
    return table.size() - 1;
}

uint32_t ConnectomeGraph::resolve_id(const Vector<uint32_t> &ids, uint64_t root_id) const {
    uint64_t lower = root_id & 0xFFFFFFFF;
    uint64_t upper = (root_id >> 32) & 0xFFFFFFFF;
    for (uint32_t i = 0; i < (uint32_t)ids.size(); i++) {
        uint64_t stored = ((uint64_t)ids[i]);
        if (stored == lower || stored == root_id || stored == upper) {
            // Check full 64-bit match
            uint64_t full = vertex_ids[i] | ((uint64_t)vertex_ids[i] << 32);
            (void)full;
        }
        if (vertex_ids[i] == (uint32_t)(root_id & 0xFFFFFFFF) &&
            (i == 0 || vertex_ids[i] == (uint32_t)(root_id >> 32) || true)) {
            // Simple linear scan for root_id match
        }
    }
    for (int i = 0; i < vertex_ids.size(); i++) {
        if (vertices[i].root_id == root_id) {
            return i;
        }
    }
    return UINT32_MAX;
}

int ConnectomeGraph::find_vertex_by_root_id(uint64_t root_id) const {
    for (int i = 0; i < vertices.size(); i++) {
        if (vertices[i].root_id == root_id) {
            return i;
        }
    }
    return -1;
}

uint64_t ConnectomeGraph::get_vertex_root_id(int idx) const {
    if (idx >= 0 && idx < vertices.size()) {
        return vertices[idx].root_id;
    }
    return 0;
}

float ConnectomeGraph::get_vertex_x(int idx) const { return (idx >= 0 && idx < vertices.size()) ? vertices[idx].x : 0.0f; }
float ConnectomeGraph::get_vertex_y(int idx) const { return (idx >= 0 && idx < vertices.size()) ? vertices[idx].y : 0.0f; }
float ConnectomeGraph::get_vertex_z(int idx) const { return (idx >= 0 && idx < vertices.size()) ? vertices[idx].z : 0.0f; }

String ConnectomeGraph::get_cell_type(int idx) const {
    if (idx >= 0 && idx < vertices.size() && vertices[idx].cell_type_idx < cell_type_table.size()) {
        return cell_type_table[vertices[idx].cell_type_idx];
    }
    return String();
}

String ConnectomeGraph::get_side(int idx) const {
    if (idx >= 0 && idx < vertices.size() && vertices[idx].side_idx < side_table.size()) {
        return side_table[vertices[idx].side_idx];
    }
    return String();
}

String ConnectomeGraph::get_nt_type(int idx) const {
    if (idx >= 0 && idx < vertices.size() && vertices[idx].nt_type_idx < nt_type_table.size()) {
        return nt_type_table[vertices[idx].nt_type_idx];
    }
    return String();
}

String ConnectomeGraph::get_flow(int idx) const {
    if (idx >= 0 && idx < vertices.size() && vertices[idx].flow_idx < flow_table.size()) {
        return flow_table[vertices[idx].flow_idx];
    }
    return String();
}

String ConnectomeGraph::get_neuropil(int edge_idx) const {
    if (edge_idx >= 0 && edge_idx < edges.size() && edges[edge_idx].neuropil_idx < neuropil_table.size()) {
        return neuropil_table[edges[edge_idx].neuropil_idx];
    }
    return String();
}

String ConnectomeGraph::get_edge_nt_type(int edge_idx) const {
    if (edge_idx >= 0 && edge_idx < edges.size() && edges[edge_idx].nt_type_idx < nt_type_table.size()) {
        return nt_type_table[edges[edge_idx].nt_type_idx];
    }
    return String();
}

float ConnectomeGraph::get_edge_synapse_count(int edge_idx) const {
    return (edge_idx >= 0 && edge_idx < edges.size()) ? edges[edge_idx].synapse_count : 0.0f;
}

int ConnectomeGraph::get_edge_pre(int edge_idx) const {
    return (edge_idx >= 0 && edge_idx < edges.size()) ? (int)edges[edge_idx].pre_idx : -1;
}

int ConnectomeGraph::get_edge_post(int edge_idx) const {
    return (edge_idx >= 0 && edge_idx < edges.size()) ? (int)edges[edge_idx].post_idx : -1;
}

int ConnectomeGraph::get_outgoing_count(int vertex_idx) const {
    return (vertex_idx >= 0 && vertex_idx < outgoing_edges.size()) ? outgoing_edges[vertex_idx].size() : 0;
}

int ConnectomeGraph::get_incoming_count(int vertex_idx) const {
    return (vertex_idx >= 0 && vertex_idx < incoming_edges.size()) ? incoming_edges[vertex_idx].size() : 0;
}

int ConnectomeGraph::get_outgoing_edge(int vertex_idx, int i) const {
    if (vertex_idx >= 0 && vertex_idx < outgoing_edges.size() && i >= 0 && i < outgoing_edges[vertex_idx].size()) {
        return (int)outgoing_edges[vertex_idx][i];
    }
    return -1;
}

int ConnectomeGraph::get_incoming_edge(int vertex_idx, int i) const {
    if (vertex_idx >= 0 && vertex_idx < incoming_edges.size() && i >= 0 && i < incoming_edges[vertex_idx].size()) {
        return (int)incoming_edges[vertex_idx][i];
    }
    return -1;
}

int ConnectomeGraph::get_max_outgoing() const {
    int max_val = 0;
    for (int i = 0; i < outgoing_edges.size(); i++) {
        if (outgoing_edges[i].size() > max_val) {
            max_val = outgoing_edges[i].size();
        }
    }
    return max_val;
}

int ConnectomeGraph::get_max_incoming() const {
    int max_val = 0;
    for (int i = 0; i < incoming_edges.size(); i++) {
        if (incoming_edges[i].size() > max_val) {
            max_val = incoming_edges[i].size();
        }
    }
    return max_val;
}

double ConnectomeGraph::get_mean_outgoing() const {
    if (outgoing_edges.size() == 0) return 0.0;
    uint64_t total = 0;
    for (int i = 0; i < outgoing_edges.size(); i++) {
        total += outgoing_edges[i].size();
    }
    return (double)total / (double)outgoing_edges.size();
}

bool ConnectomeGraph::load_csv(const String &vertices_path, const String &edges_path, const String &somas_path) {
    clear();

    // Phase 1: Load somas (root_id, x, y, z)
    print_line("[Connectome] Loading somas from: " + somas_path);
    std::ifstream somas_file(somas_path.utf8().get_data());
    if (!somas_file.is_open()) {
        print_line("[Connectome] ERROR: Cannot open somas file: " + somas_path);
        return false;
    }

    std::string line;
    std::getline(somas_file, line); // Skip header

    struct TempSoma {
        uint64_t root_id;
        float x, y, z;
    };
    Vector<TempSoma> temp_somas;

    while (std::getline(somas_file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        TempSoma s;
        if (true) {
            std::getline(ss, cell, ','); s.root_id = std::stoull(cell);
            std::getline(ss, cell, ','); s.x = std::stof(cell);
            std::getline(ss, cell, ','); s.y = std::stof(cell);
            std::getline(ss, cell, ','); s.z = std::stof(cell);
            temp_somas.push_back(s);
        } else {
            continue;
        }
    }
    print_line("[Connectome] Loaded " + itos(temp_somas.size()) + " soma positions");

    // Phase 2: Load vertices (cell_types: root_id, cell_type, side, nt_type, flow)
    print_line("[Connectome] Loading vertices from: " + vertices_path);
    std::ifstream verts_file(vertices_path.utf8().get_data());
    if (!verts_file.is_open()) {
        print_line("[Connectome] ERROR: Cannot open vertices file: " + vertices_path);
        return false;
    }

    std::getline(verts_file, line); // Skip header
    vertices.reserve(140000);

    // Build soma lookup
    Vector<uint64_t> soma_ids;
    for (int i = 0; i < temp_somas.size(); i++) {
        soma_ids.push_back(temp_somas[i].root_id);
    }

    while (std::getline(verts_file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        ConnectomeVertex v;

        if (true) {
            std::getline(ss, cell, ',');
            v.root_id = std::stoull(cell);

            std::getline(ss, cell, ',');
            v.cell_type_idx = resolve_string(cell_type_table, String(cell.c_str()));

            std::getline(ss, cell, ',');
            v.side_idx = resolve_string(side_table, String(cell.c_str()));

            std::getline(ss, cell, ',');
            v.nt_type_idx = resolve_string(nt_type_table, String(cell.c_str()));

            std::getline(ss, cell, ',');
            v.flow_idx = resolve_string(flow_table, String(cell.c_str()));

            // Find corresponding soma position
            v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
            for (int si = 0; si < temp_somas.size(); si++) {
                if (temp_somas[si].root_id == v.root_id) {
                    v.x = temp_somas[si].x;
                    v.y = temp_somas[si].y;
                    v.z = temp_somas[si].z;
                    break;
                }
            }

            vertices.push_back(v);
        } else {
            continue;
        }
    }

    // Free temp somas now that we've extracted positions
    temp_somas.clear();
    soma_ids.clear();

    print_line("[Connectome] Loaded " + itos(vertices.size()) + " vertices");

    // Phase 3: Load edges (pre_id, post_id, synapse_count, neuropil, nt_type)
    print_line("[Connectome] Loading edges from: " + edges_path);
    std::ifstream edges_file(edges_path.utf8().get_data());
    if (!edges_file.is_open()) {
        print_line("[Connectome] ERROR: Cannot open edges file: " + edges_path);
        return false;
    }

    std::getline(edges_file, line); // Skip header
    edges.reserve(4000000);

    while (std::getline(edges_file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        ConnectomeEdge e;

        if (true) {
            std::getline(ss, cell, ',');
            uint64_t pre_root_id = std::stoull(cell);
            e.pre_idx = (uint32_t)find_vertex_by_root_id(pre_root_id);
            if (e.pre_idx == UINT32_MAX) continue;

            std::getline(ss, cell, ',');
            uint64_t post_root_id = std::stoull(cell);
            e.post_idx = (uint32_t)find_vertex_by_root_id(post_root_id);
            if (e.post_idx == UINT32_MAX) continue;

            std::getline(ss, cell, ',');
            e.synapse_count = std::stof(cell);

            std::getline(ss, cell, ',');
            e.neuropil_idx = resolve_string(neuropil_table, String(cell.c_str()));

            std::getline(ss, cell, ',');
            e.nt_type_idx = resolve_string(nt_type_table, String(cell.c_str()));

            edges.push_back(e);
        } else {
            continue;
        }
    }

    print_line("[Connectome] Loaded " + itos(edges.size()) + " edges");

    // Phase 4: Build adjacency lists
    print_line("[Connectome] Building adjacency lists...");
    outgoing_edges.resize(vertices.size());
    incoming_edges.resize(vertices.size());

    for (uint32_t ei = 0; ei < (uint32_t)edges.size(); ei++) {
        uint32_t pre = edges[ei].pre_idx;
        uint32_t post = edges[ei].post_idx;
        if (pre < (uint32_t)outgoing_edges.size()) {
            outgoing_edges.write[pre].push_back(ei);
        }
        if (post < (uint32_t)incoming_edges.size()) {
            incoming_edges.write[post].push_back(ei);
        }
    }

    print_line("[Connectome] Done. Max outgoing: " + itos(get_max_outgoing()) +
               ", Max incoming: " + itos(get_max_incoming()) +
               ", Mean degree: " + rtos(get_mean_outgoing()));

    return true;
}

bool ConnectomeGraph::save_binary(const String &path) const {
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    if (file.is_null()) {
        print_line("[Connectome] ERROR: Cannot write: " + path);
        return false;
    }

    file->store_32(0x55F5);
    file->store_32(2); // version 2 adds tetrahedra
    file->store_32(vertices.size());
    file->store_32(edges.size());
    file->store_32(tetrahedra.size());

    for (int i = 0; i < vertices.size(); i++) {
        file->store_64(vertices[i].root_id);
        file->store_float(vertices[i].x);
        file->store_float(vertices[i].y);
        file->store_float(vertices[i].z);
        file->store_16(vertices[i].cell_type_idx);
        file->store_16(vertices[i].side_idx);
        file->store_16(vertices[i].nt_type_idx);
        file->store_16(vertices[i].flow_idx);
    }

    for (int i = 0; i < edges.size(); i++) {
        file->store_32(edges[i].pre_idx);
        file->store_32(edges[i].post_idx);
        file->store_float(edges[i].synapse_count);
        file->store_16(edges[i].neuropil_idx);
        file->store_16(edges[i].nt_type_idx);
    }

    for (int i = 0; i < tetrahedra.size(); i++) {
        file->store_32(tetrahedra[i].v0);
        file->store_32(tetrahedra[i].v1);
        file->store_32(tetrahedra[i].v2);
        file->store_32(tetrahedra[i].v3);
    }

    auto write_str_table = [&](const Vector<String> &table) {
        file->store_32(table.size());
        for (int i = 0; i < table.size(); i++) {
            CharString cs = table[i].utf8();
            file->store_32(cs.length());
            file->store_buffer((const uint8_t *)cs.get_data(), cs.length());
        }
    };

    write_str_table(cell_type_table);
    write_str_table(side_table);
    write_str_table(nt_type_table);
    write_str_table(flow_table);
    write_str_table(neuropil_table);

    return true;
}

bool ConnectomeGraph::load_binary(const String &path) {
    clear();

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null()) {
        print_line("[Connectome] ERROR: Cannot open: " + path);
        return false;
    }

    uint32_t magic = file->get_32();
    if (magic != 0x55F5) {
        print_line("[Connectome] ERROR: Bad magic: " + itos(magic));
        return false;
    }

    uint32_t version = file->get_32();
    uint32_t num_verts = file->get_32();
    uint32_t num_edges = file->get_32();
    uint32_t num_tets = (version >= 2) ? file->get_32() : 0;

    vertices.resize(num_verts);
    edges.resize(num_edges);

    // Read vertex data byte by byte to avoid FileAccess API issues
    for (uint32_t i = 0; i < num_verts; i++) {
        vertices.write[i].root_id = file->get_64();
        vertices.write[i].x = file->get_float();
        vertices.write[i].y = file->get_float();
        vertices.write[i].z = file->get_float();
        vertices.write[i].cell_type_idx = file->get_16();
        vertices.write[i].side_idx = file->get_16();
        vertices.write[i].nt_type_idx = file->get_16();
        vertices.write[i].flow_idx = file->get_16();
    }

    for (uint32_t i = 0; i < num_edges; i++) {
        edges.write[i].pre_idx = file->get_32();
        edges.write[i].post_idx = file->get_32();
        edges.write[i].synapse_count = file->get_float();
        edges.write[i].neuropil_idx = file->get_16();
        edges.write[i].nt_type_idx = file->get_16();
    }

    tetrahedra.resize(num_tets);
    for (uint32_t i = 0; i < num_tets; i++) {
        tetrahedra.write[i].v0 = file->get_32();
        tetrahedra.write[i].v1 = file->get_32();
        tetrahedra.write[i].v2 = file->get_32();
        tetrahedra.write[i].v3 = file->get_32();
    }

    auto read_str_table = [&](Vector<String> &table) {
        uint32_t count = file->get_32();
        table.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            uint32_t len = file->get_32();
            if (len > 0 && len < 256) {
                Vector<uint8_t> buf;
                buf.resize(len + 1);
                file->get_buffer(buf.ptrw(), len);
                buf.write[len] = 0;
                table.write[i] = String((const char *)buf.ptr());
            } else {
                print_line("[Connectome] ERROR: Bad string length " + itos(len) + " at pos=" + itos(file->get_position()));
                table.write[i] = "unknown";
            }
        }
    };

    read_str_table(cell_type_table);
    read_str_table(side_table);
    read_str_table(nt_type_table);
    read_str_table(flow_table);
    read_str_table(neuropil_table);

    // Rebuild adjacency
    outgoing_edges.resize(vertices.size());
    incoming_edges.resize(vertices.size());

    for (uint32_t ei = 0; ei < (uint32_t)edges.size(); ei++) {
        uint32_t pre = edges[ei].pre_idx;
        uint32_t post = edges[ei].post_idx;
        if (pre < (uint32_t)outgoing_edges.size()) {
            outgoing_edges.write[pre].push_back(ei);
        }
        if (post < (uint32_t)incoming_edges.size()) {
            incoming_edges.write[post].push_back(ei);
        }
    }

    print_line("[Connectome] Loaded binary: " + itos(num_verts) + " vertices, " + itos(num_edges) + " edges");
    return true;
}

void ConnectomeGraph::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_csv", "vertices_path", "edges_path", "somas_path"), &ConnectomeGraph::load_csv);
    ClassDB::bind_method(D_METHOD("load_binary", "path"), &ConnectomeGraph::load_binary);
    ClassDB::bind_method(D_METHOD("load_with_tets", "path"), &ConnectomeGraph::load_with_tets);
    ClassDB::bind_method(D_METHOD("save_binary", "path"), &ConnectomeGraph::save_binary);
    ClassDB::bind_method(D_METHOD("clear"), &ConnectomeGraph::clear);

    ClassDB::bind_method(D_METHOD("get_vertex_count"), &ConnectomeGraph::get_vertex_count);
    ClassDB::bind_method(D_METHOD("get_edge_count"), &ConnectomeGraph::get_edge_count);
    ClassDB::bind_method(D_METHOD("get_tetrahedron_count"), &ConnectomeGraph::get_tetrahedron_count);
    ClassDB::bind_method(D_METHOD("get_tet_vertex", "tet_idx", "vertex_slot"), &ConnectomeGraph::get_tet_vertex);

    ClassDB::bind_method(D_METHOD("find_vertex_by_root_id", "root_id"), &ConnectomeGraph::find_vertex_by_root_id);
    ClassDB::bind_method(D_METHOD("get_vertex_root_id", "idx"), &ConnectomeGraph::get_vertex_root_id);
    ClassDB::bind_method(D_METHOD("get_vertex_x", "idx"), &ConnectomeGraph::get_vertex_x);
    ClassDB::bind_method(D_METHOD("get_vertex_y", "idx"), &ConnectomeGraph::get_vertex_y);
    ClassDB::bind_method(D_METHOD("get_vertex_z", "idx"), &ConnectomeGraph::get_vertex_z);

    ClassDB::bind_method(D_METHOD("get_cell_type", "idx"), &ConnectomeGraph::get_cell_type);
    ClassDB::bind_method(D_METHOD("get_side", "idx"), &ConnectomeGraph::get_side);
    ClassDB::bind_method(D_METHOD("get_nt_type", "idx"), &ConnectomeGraph::get_nt_type);
    ClassDB::bind_method(D_METHOD("get_edge_nt_type", "edge_idx"), &ConnectomeGraph::get_edge_nt_type);
    ClassDB::bind_method(D_METHOD("get_flow", "idx"), &ConnectomeGraph::get_flow);
    ClassDB::bind_method(D_METHOD("get_neuropil", "edge_idx"), &ConnectomeGraph::get_neuropil);

    ClassDB::bind_method(D_METHOD("get_edge_synapse_count", "edge_idx"), &ConnectomeGraph::get_edge_synapse_count);
    ClassDB::bind_method(D_METHOD("get_edge_pre", "edge_idx"), &ConnectomeGraph::get_edge_pre);
    ClassDB::bind_method(D_METHOD("get_edge_post", "edge_idx"), &ConnectomeGraph::get_edge_post);

    ClassDB::bind_method(D_METHOD("get_outgoing_count", "vertex_idx"), &ConnectomeGraph::get_outgoing_count);
    ClassDB::bind_method(D_METHOD("get_incoming_count", "vertex_idx"), &ConnectomeGraph::get_incoming_count);
    ClassDB::bind_method(D_METHOD("get_outgoing_edge", "vertex_idx", "i"), &ConnectomeGraph::get_outgoing_edge);
    ClassDB::bind_method(D_METHOD("get_incoming_edge", "vertex_idx", "i"), &ConnectomeGraph::get_incoming_edge);

    ClassDB::bind_method(D_METHOD("get_max_outgoing"), &ConnectomeGraph::get_max_outgoing);
    ClassDB::bind_method(D_METHOD("get_max_incoming"), &ConnectomeGraph::get_max_incoming);
    ClassDB::bind_method(D_METHOD("get_mean_outgoing"), &ConnectomeGraph::get_mean_outgoing);
}
