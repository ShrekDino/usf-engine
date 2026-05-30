#!/usr/bin/env python3
"""
FlyWire Connectome Import Script
=================================
Downloads and converts the FAFB v783 connectome into the C++ ConnectomeGraph
binary format for the USF Engine.

Usage:
    python3 import_flywire.py [--dataset fafb] [--output connectome_data/]

Requires: pip install caveclient requests tqdm
Optional: A FlyWire/CAVE auth token (for API access)
"""

import argparse
import csv
import json
import os
import struct
import sys
import time
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

FAFB_V783_INFO = {
    "name": "FAFB v783",
    "neurons": 139255,
    "connections": 3732460,
    "dataset": "flywire_fafb",
    "codex_url": "https://codex.flywire.ai/api/download_resource",
}

# Expected CSV files from Codex static downloads
CSV_FILES = {
    "neurons": {
        "product": "consolidated_cell_types",
        "columns": ["root_id", "cell_type", "side", "nt_type", "flow"],
    },
    "connections": {
        "product": "connections_princeton",
        "columns": ["pre_id", "post_id", "synapse_count", "neuropil", "nt_type"],
    },
    "somas": {
        "product": "soma_locations",
        "columns": ["root_id", "x", "y", "z"],
    },
}

# Binary format magic
MAGIC = 0x55F5
VERSION = 1

# ---------------------------------------------------------------------------
# CSV Download
# ---------------------------------------------------------------------------


def download_cvs(codex_url, product, dataset, output_dir):
    """Download a CSV from Codex static endpoint."""
    url = f"{codex_url}?data_product={product}&dataset={dataset}"
    out_path = output_dir / f"{dataset}_{product}.csv"
    if out_path.exists():
        print(f"  [SKIP] {out_path.name} exists")
        return str(out_path)

    print(f"  Downloading {product}...")
    import requests

    resp = requests.get(url, stream=True, timeout=300)
    if resp.status_code != 200:
        print(f"  [FAIL] HTTP {resp.status_code}: {resp.reason}")
        print(f"  Please download manually from {url}")
        return None

    total = int(resp.headers.get("content-length", 0))
    downloaded = 0
    with open(out_path, "wb") as f:
        for chunk in resp.iter_content(chunk_size=8192):
            f.write(chunk)
            downloaded += len(chunk)
            if total > 0:
                pct = int(100 * downloaded / total)
                sys.stdout.write(f"\r    {pct}% ({downloaded // 1024 // 1024} MB)")
                sys.stdout.flush()
    print()
    print(f"  [OK] {out_path.stat().st_size // 1024 // 1024} MB")
    return str(out_path)


def download_all(output_dir, dataset="fafb"):
    """Download all three CSV files."""
    print(f"[Connectome] Downloading {dataset} dataset...")
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    paths = {}
    for name, info in CSV_FILES.items():
        path = download_cvs(
            FAFB_V783_INFO["codex_url"],
            info["product"],
            dataset,
            output_dir,
        )
        paths[name] = path
        if path is None:
            print(f"  [WARN] {name} not downloaded")

    return paths


# ---------------------------------------------------------------------------
# CSV → Binary Conversion
# ---------------------------------------------------------------------------


def convert_csv_to_binary(csv_paths, dataset, output_path):
    """Convert downloaded CSVs to the connectome binary format."""
    print(f"\n[Connectome] Converting {dataset} to binary format...")

    # Phase 1: Load somas
    print("  Phase 1/4: Loading soma positions...")
    somas = {}
    soma_count = 0
    with open(csv_paths["somas"], "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            root_id = int(row["root_id"])
            somas[root_id] = (
                float(row.get("x", 0)),
                float(row.get("y", 0)),
                float(row.get("z", 0)),
            )
            soma_count += 1
    print(f"    {soma_count} somas loaded")

    # Phase 2: Load vertices
    print("  Phase 2/4: Loading vertices...")
    vertices = []
    cell_types = {}
    sides = {}
    nt_types = {}
    flows = {}

    with open(csv_paths["neurons"], "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            root_id = int(row["root_id"])
            cell_type = row.get("cell_type", "unknown")
            side = row.get("side", "center")
            nt_type = row.get("nt_type", "unknown")
            flow = row.get("flow", "unknown")
            x, y, z = somas.get(root_id, (0.0, 0.0, 0.0))

            vertices.append((root_id, cell_type, side, nt_type, flow, x, y, z))

            if cell_type not in cell_types:
                cell_types[cell_type] = len(cell_types)
            if side not in sides:
                sides[side] = len(sides)
            if nt_type not in nt_types:
                nt_types[nt_type] = len(nt_types)
            if flow not in flows:
                flows[flow] = len(flows)

    print(f"    {len(vertices)} vertices loaded")
    print(f"    {len(cell_types)} cell types, {len(sides)} sides, {len(nt_types)} NT types, {len(flows)} flows")

    # Build vertex lookup
    vertex_map = {v[0]: i for i, v in enumerate(vertices)}

    # Phase 3: Load edges
    print("  Phase 3/4: Loading edges...")
    neuropils = {}
    edges = []
    skipped = 0

    with open(csv_paths["connections"], "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            pre_id = int(row["pre_id"])
            post_id = int(row["post_id"])

            if pre_id not in vertex_map or post_id not in vertex_map:
                skipped += 1
                continue

            pre_idx = vertex_map[pre_id]
            post_idx = vertex_map[post_id]
            synapse_count = float(row.get("synapse_count", 1))
            neuropil = row.get("neuropil", "unknown")
            nt_type = row.get("nt_type", "unknown")

            edges.append((pre_idx, post_idx, synapse_count, neuropil, nt_type))

            if neuropil not in neuropils:
                neuropils[neuropil] = len(neuropils)
            if nt_type not in nt_types:
                nt_types[nt_type] = len(nt_types)

    print(f"    {len(edges)} edges loaded ({skipped} skipped)")

    # Phase 4: Write binary
    print("  Phase 4/4: Writing binary file...")

    # Build string tables
    cell_type_list = [""] * len(cell_types)
    for name, idx in cell_types.items():
        cell_type_list[idx] = name
    side_list = [""] * len(sides)
    for name, idx in sides.items():
        side_list[idx] = name
    nt_type_list = [""] * len(nt_types)
    for name, idx in nt_types.items():
        nt_type_list[idx] = name
    flow_list = [""] * len(flows)
    for name, idx in flows.items():
        flow_list[idx] = name
    neuropil_list = [""] * len(neuropils)
    for name, idx in neuropils.items():
        neuropil_list[idx] = name

    with open(output_path, "wb") as f:
        # Header
        f.write(struct.pack("<IIII", MAGIC, VERSION, len(vertices), len(edges)))

        # Vertices
        for v in vertices:
            root_id, ct, sd, nt, fl, x, y, z = v
            f.write(struct.pack("<QfffHHHH",
                                root_id, x, y, z,
                                cell_types[ct],
                                sides[sd],
                                nt_types[nt],
                                flows[fl]))

        # Edges
        for e in edges:
            pre, post, w, np, nt = e
            f.write(struct.pack("<IIfHH",
                                pre, post, w,
                                neuropils[np],
                                nt_types[nt]))

        # String tables
        def write_str_table(f, table):
            f.write(struct.pack("<I", len(table)))
            for s in table:
                encoded = s.encode("utf-8")
                f.write(struct.pack("<I", len(encoded)))
                f.write(encoded)

        write_str_table(f, cell_type_list)
        write_str_table(f, side_list)
        write_str_table(f, nt_type_list)
        write_str_table(f, flow_list)
        write_str_table(f, neuropil_list)

    file_size = os.path.getsize(output_path)
    print(f"\n[Connectome] Wrote {output_path}")
    print(f"  Size: {file_size // 1024 // 1024} MB")
    print(f"  Vertices: {len(vertices)}")
    print(f"  Edges: {len(edges)}")
    print(f"  String tables: {len(cell_types)} cell types, {len(sides)} sides, "
          f"{len(nt_types)} NT types, {len(flows)} flows, {len(neuropils)} neuropils")

    # Stats
    if edges:
        outgoing = {}
        incoming = {}
        for e in edges:
            pre, post, w, np, nt = e
            outgoing[pre] = outgoing.get(pre, 0) + 1
            incoming[post] = incoming.get(post, 0) + 1
        max_out = max(outgoing.values()) if outgoing else 0
        max_in = max(incoming.values()) if incoming else 0
        mean_out = sum(outgoing.values()) / len(outgoing) if outgoing else 0
        print(f"  Max outgoing: {max_out}, Max incoming: {max_in}, Mean degree: {mean_out:.2f}")

    return output_path


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description="Import FlyWire connectome for USF Engine")
    parser.add_argument("--dataset", default="fafb", help="Dataset name (default: fafb)")
    parser.add_argument("--output", default="connectome_data", help="Output directory")
    parser.add_argument("--download", action="store_true", help="Download CSVs from Codex")
    parser.add_argument("--neurons", help="Path to neurons CSV (skip download)")
    parser.add_argument("--connections", help="Path to connections CSV (skip download)")
    parser.add_argument("--somas", help="Path to somas CSV (skip download)")
    parser.add_argument("--generate-sample", action="store_true",
                        help="Generate a small sample (100 neurons) for testing")
    args = parser.parse_args()

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    if args.generate_sample:
        print("[Connectome] Generating sample data for testing...")
        sample_path = output_dir / f"{args.dataset}_sample.bin"
        _generate_sample(sample_path)
        return

    # Get CSV paths
    csv_paths = {}
    if args.download:
        csv_paths = download_all(output_dir, args.dataset)
    elif args.neurons and args.connections and args.somas:
        csv_paths = {
            "neurons": args.neurons,
            "connections": args.connections,
            "somas": args.somas,
        }
    else:
        # Check if cached files exist
        for name in CSV_FILES:
            candidate = output_dir / f"{args.dataset}_{CSV_FILES[name]['product']}.csv"
            if candidate.exists():
                csv_paths[name] = str(candidate)
                print(f"[Connectome] Found cached: {candidate.name}")

    if len(csv_paths) < 3:
        print("\n[Connectome] Missing CSV files. Options:")
        print("  1. --download  (requires internet + Codex auth)")
        print("  2. --neurons X --connections Y --somas Z  (provide local paths)")
        print("  3. --generate-sample  (create test data)")
        return

    # Convert to binary
    binary_path = output_dir / f"{args.dataset}_v783.bin"
    result = convert_csv_to_binary(csv_paths, args.dataset, str(binary_path))

    print(f"\n[Connectome] Done. Import with GDScript:")
    print(f"  var cg = ConnectomeGraph.new()")
    print(f"  cg.load_binary('{result}')")
    print(f"  print(cg.get_vertex_count(), 'vertices,', cg.get_edge_count(), 'edges')")


def _generate_sample(output_path):
    """Generate a small random sample for testing."""
    import random
    n_neurons = 100
    n_edges = 500

    cell_types_list = ["KC", "MBON", "DAN", "OAN", "LHN", "LC4", "T4a", "Tm1", "Mi1", "L1"]
    sides_list = ["left", "right", "center"]
    nt_types_list = ["acetylcholine", "gaba", "glutamate", "dopamine", "octopamine"]
    flows_list = ["visual", "olfactory", "auditory", "motor", "central"]
    neuropils_list = ["LO", "ME", "AL", "CX", "LH", "FB", "NO", "EB", "PB", "BU"]

    vertices = []
    for i in range(n_neurons):
        root_id = 72057594060000000 + i
        vertices.append((
            root_id,
            random.choice(cell_types_list),
            random.choice(sides_list),
            random.choice(nt_types_list),
            random.choice(flows_list),
            random.uniform(0, 500000),
            random.uniform(0, 300000),
            random.uniform(0, 7000),
        ))

    vertex_map = {v[0]: i for i, v in enumerate(vertices)}

    edges = []
    for _ in range(n_edges):
        pre = random.randint(0, n_neurons - 1)
        post = random.randint(0, n_neurons - 1)
        if pre == post:
            continue
        weight = random.randint(1, 20)
        edges.append((
            pre, post, float(weight),
            random.choice(neuropils_list),
            random.choice(nt_types_list),
        ))

    with open(output_path, "wb") as f:
        f.write(struct.pack("<IIII", MAGIC, VERSION, len(vertices), len(edges)))
        for v in vertices:
            rid, ct, sd, nt, fl, x, y, z = v
            f.write(struct.pack("<QfffHHHH", rid, x, y, z,
                                cell_types_list.index(ct),
                                sides_list.index(sd),
                                nt_types_list.index(nt),
                                flows_list.index(fl)))

        for e in edges:
            pre, post, w, np, nt = e
            f.write(struct.pack("<IIfHH", pre, post, w,
                                neuropils_list.index(np),
                                nt_types_list.index(nt)))

        for table in [cell_types_list, sides_list, nt_types_list, flows_list, neuropils_list]:
            f.write(struct.pack("<I", len(table)))
            for s in table:
                encoded = s.encode("utf-8")
                f.write(struct.pack("<I", len(encoded)))
                f.write(encoded)

    print(f"[Connectome] Generated sample: {output_path} ({n_neurons} neurons, {len(edges)} edges)")


if __name__ == "__main__":
    main()
