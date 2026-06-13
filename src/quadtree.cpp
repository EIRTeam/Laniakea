#include "quadtree.h"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/json.hpp"
#include "godot_cpp/core/error_macros.hpp"

void QuadTree::clear() {
    nodes.clear();
    nodes.push_back({
        .rect = Rect2i(Vector2i(), Vector2i(tree_extent, tree_extent))
    });
}

void QuadTree::insert_camera(const Vector2i p_camera_pos) {
    _try_camera_subdiv(p_camera_pos, 0);
    constraint_lods(0);
}

Ref<QuadTree> QuadTree::create(int p_extent, int p_max_depth, float p_threshold_multiplier) {
	Ref<QuadTree> qt;
    qt.instantiate();
    qt->tree_extent = p_extent;
    qt->max_depth = p_max_depth;
    qt->threshold_multiplier = p_threshold_multiplier;
    qt->clear();
    return qt;
}

Rect2i QuadTree::get_node_rect_bind(int p_node_dix) const {
	return nodes[p_node_dix].rect;
}

int QuadTree::get_greater_or_equal_neighbor(const int p_node_idx, const NodeDirection p_direction) const {
    if (nodes[p_node_idx].parent == -1) {
        return -1;
    }

    ERR_FAIL_INDEX_V(p_node_idx, nodes.size(), -1);

    const NodePosition *positions = POSITIONS_IN_DIRECTION[p_direction];
    const NodePosition *opposite_positions = POSITIONS_IN_DIRECTION[OPPOSITE_DIRECTIONS[p_direction]];

    const TreeNode &node = nodes[p_node_idx];
    const TreeNode &parent = nodes[node.parent];
    for (int i = 0; i < 2; i++) {

        if (parent.children[opposite_positions[i]] == p_node_idx) {
            return parent.children[positions[i]];
        }
    }

    const int neighbor_node_idx = get_greater_or_equal_neighbor(node.parent, p_direction);
    if (neighbor_node_idx == -1 || nodes[neighbor_node_idx].is_leaf()) {
        return neighbor_node_idx;
    }
    const TreeNode &neighbor_node = nodes[neighbor_node_idx];

    return parent.children[positions[0]] == p_node_idx ? neighbor_node.children[opposite_positions[0]] : neighbor_node.children[opposite_positions[1]];
}

void QuadTree::get_neighbors(const int p_node_idx, LocalVector<NeighborData> &r_out) const {
    ERR_FAIL_INDEX(p_node_idx, nodes.size());
    for (int i = 0; i < NodeDirection::DIRECTION_MAX; i++) {
        const int greater_neighbor = get_greater_or_equal_neighbor(p_node_idx, (NodeDirection)i);
        if (greater_neighbor == -1) {
            continue;
        }

        const LocalVector<int> local_neighbors = get_local_neighbors(greater_neighbor, OPPOSITE_DIRECTIONS[i]);
        for (int neighbor_idx : local_neighbors) {
            r_out.push_back({
                .neighbor_idx = neighbor_idx,
                .direction = (NodeDirection)i
            });
        }
    }
}

bool QuadTree::can_subdivide_node(const TreeNode &p_node) const {
	return p_node.depth+1 < max_depth;
}

Vector2i QuadTree::get_position_in_parent(const TreeNode &p_parent, const NodePosition p_position) const {
	int depth_size = get_depth_size(p_parent.depth+1);

	Vector2i pos_in_parent;
	switch (p_position) {
		case NodePosition::POSITION_NW: {
			pos_in_parent = Vector2i();
        } break;
		case NodePosition::POSITION_NE: {
			pos_in_parent = Vector2i(depth_size, 0);
        } break;
		case NodePosition::POSITION_SE: {
			pos_in_parent = Vector2i(depth_size, depth_size);
        } break;
		case NodePosition::POSITION_SW: {
			pos_in_parent = Vector2i(0, depth_size);
        }
    }

	return pos_in_parent;
}

int QuadTree::get_depth_size(int p_depth) const {

	return tree_extent >> p_depth;
}

void QuadTree::subdivide_node(int p_idx) {
	ERR_FAIL_INDEX(p_idx, nodes.size());
    DEV_ASSERT(nodes[p_idx].is_leaf());
    const int depth_size = get_depth_size(nodes[p_idx].depth+1);
    for (int i = 0; i < NodePosition::POSITION_MAX; i++) {
        TreeNode &parent_node = nodes[p_idx];
        parent_node.children[i] = nodes.size();
        nodes.push_back(TreeNode {
            .parent = p_idx,
            .depth = parent_node.depth+1,
            .rect = Rect2i(parent_node.rect.position + get_position_in_parent(parent_node, (NodePosition)i), Vector2i(depth_size, depth_size))
        });
    }
}

Vector2i QuadTree::calculate_node_center(const TreeNode &p_node) const {
	return p_node.rect.position + Vector2i(1, 1) * (get_depth_size(p_node.depth) / 2);
}

void QuadTree::_try_camera_subdiv(const Vector2i p_camera_pos, int p_node_idx) {
    if (!can_subdivide_node(nodes[p_node_idx])) {
        return;
    }
    const float threshold = (get_depth_size(nodes[p_node_idx].depth) * Math_SQRT2 * threshold_multiplier);
    const Vector2i node_center = calculate_node_center(nodes[p_node_idx]);
    if (node_center.distance_to(p_camera_pos) > threshold) {
        return;
    }

    subdivide_node(p_node_idx);

    for (int i = 0; i < NodePosition::POSITION_MAX; i++) {
        const int child = nodes[p_node_idx].children[i];
        _try_camera_subdiv(p_camera_pos, child);
    }
}

void QuadTree::constraint_lods(const int p_node_idx) {
    if (nodes[p_node_idx].is_leaf()) {
        LocalVector<NeighborData> neighbors;
        get_neighbors(p_node_idx, neighbors);

        for (const NeighborData &neighbor : neighbors) {
            const TreeNode &node = nodes[p_node_idx];
            const TreeNode &neighbor_node = nodes[neighbor.neighbor_idx];
            if (Math::abs(neighbor_node.depth - nodes[p_node_idx].depth) <= 1) {
                continue;
            }

            const int lowest_lod_node_idx = node.depth >= neighbor_node.depth ? neighbor.neighbor_idx : p_node_idx;

            if (can_subdivide_node(nodes[lowest_lod_node_idx])) {
                subdivide_node(lowest_lod_node_idx);
                constraint_lods(lowest_lod_node_idx);
                break;
            }
        }
    }

    const int *children = nodes[p_node_idx].children;
    for (int i = 0; i < std::size(nodes[p_node_idx].children); i++) {
        if (children[i] == -1) {
            continue;
        }
        constraint_lods(children[i]);
    }
}

LocalVector<int> QuadTree::get_local_neighbors(const int p_node_idx, const NodeDirection p_direction) const {
    LocalVector<int> local_neighbors;
    List<int> candidates;

    const NodePosition *positions = POSITIONS_IN_DIRECTION[p_direction];

    candidates.push_back(p_node_idx);

    while (!candidates.is_empty()) {
        const int candidate_idx = candidates.front()->get();
        candidates.pop_front();

        if (nodes[candidate_idx].is_leaf()) {
            local_neighbors.push_back(candidate_idx);
        } else {
            for (int i = 0; i < 2; i++) {
                candidates.push_back(nodes[candidate_idx].children[positions[i]]);
            }
        }
    }

    return local_neighbors;
}

TypedArray<Dictionary> QuadTree::get_leafs_bind() const {
	TypedArray<Dictionary> leafs;

    for (int i = 0; i < nodes.size(); i++) {
        const TreeNode &node = nodes[i];
        if (node.is_leaf()) {
            Dictionary dict;
            dict[StringName("rect")] = node.rect;
            dict[StringName("depth")] = node.depth;
            dict[StringName("idx")] = i;

            int lod_mask = 0;

            LocalVector<NeighborData> neighbors;
            get_neighbors(i, neighbors);

            for (const NeighborData &neighbor : neighbors) {
                if (nodes[neighbor.neighbor_idx].depth < node.depth) {
                    switch(neighbor.direction) {
                        case NodeDirection::DIRECTION_N: {
                            lod_mask |= LODMesh::LODMask::LOD_MASK_N;
                        } break;
                        case NodeDirection::DIRECTION_S: {
                            lod_mask |= LODMesh::LODMask::LOD_MASK_S;
                        } break;
                        case NodeDirection::DIRECTION_E: {
                            lod_mask |= LODMesh::LODMask::LOD_MASK_E;
                        } break;
                        case NodeDirection::DIRECTION_W: {
                            lod_mask |= LODMesh::LODMask::LOD_MASK_W;
                        } break;
                    }
                }
            }

            dict[StringName("lod_mask")] = lod_mask;

            leafs.push_back(dict);
        }
    }

    return leafs;
}

void QuadTree::_bind_methods() {
    ClassDB::bind_static_method("QuadTree",  D_METHOD("create", "extent", "max_depth"), &QuadTree::create);
    ClassDB::bind_method(D_METHOD("insert_camera", "camera_pos"), &QuadTree::insert_camera);
    ClassDB::bind_method(D_METHOD("get_leafs"), &QuadTree::get_leafs_bind);
    ClassDB::bind_method(D_METHOD("get_node_rect"), &QuadTree::get_node_rect_bind);
    ClassDB::bind_method(D_METHOD("get_node_neighbors"), &QuadTree::get_node_neighbors_bind);
}

PackedInt32Array QuadTree::get_node_neighbors_bind(int p_node_dix) const {
	LocalVector<NeighborData> neighbors;
    get_neighbors(p_node_dix, neighbors);

    PackedInt32Array p;
    p.resize(neighbors.size());

    for (int i = 0; i < neighbors.size(); i++) {
        p[i] = neighbors[i].neighbor_idx;
    }

    return p;
}

void QuadTree::get_leafs(LocalVector<LeafInformation> &r_out_leaves) const {
    for (int i = 0; i < nodes.size(); i++) {
        if (!nodes[i].is_leaf()) {
            continue;
        }
        LocalVector<NeighborData> neighbors;
        get_neighbors(i, neighbors);

        BitField<LODMesh::LODMask> lod_mask = 0;

        for (const NeighborData &neighbor : neighbors) {
            if (nodes[neighbor.neighbor_idx].depth < nodes[i].depth) {
                switch(neighbor.direction) {
                    case NodeDirection::DIRECTION_N: {
                        lod_mask.set_flag(LODMesh::LODMask::LOD_MASK_N);
                    } break;
                    case NodeDirection::DIRECTION_S: {
                        lod_mask.set_flag(LODMesh::LODMask::LOD_MASK_S);
                    } break;
                    case NodeDirection::DIRECTION_E: {
                        lod_mask.set_flag(LODMesh::LODMask::LOD_MASK_E);
                    } break;
                    case NodeDirection::DIRECTION_W: {
                        lod_mask.set_flag(LODMesh::LODMask::LOD_MASK_W);
                    } break;
                }
            }
        }

        r_out_leaves.push_back({
            .rect = nodes[i].rect,
            .lod_mask = lod_mask,
            .depth = nodes[i].depth
        });
    }
}

QuadTree::QuadTree() {
}
