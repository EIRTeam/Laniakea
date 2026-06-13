#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "lod_mesh.h"

using namespace godot;

class QuadTree : public RefCounted {
    GDCLASS(QuadTree, RefCounted);

    enum NodePosition {
        POSITION_NW,
		POSITION_NE,
		POSITION_SW,
		POSITION_SE,
        POSITION_MAX
    };

    enum NodeDirection {
        DIRECTION_N,
        DIRECTION_S,
        DIRECTION_E,
        DIRECTION_W,
        DIRECTION_MAX
    };

    static constexpr NodeDirection OPPOSITE_DIRECTIONS[4] = {
        NodeDirection::DIRECTION_S,
	    NodeDirection::DIRECTION_N,
	    NodeDirection::DIRECTION_W,
	    NodeDirection::DIRECTION_E
    };

    static constexpr NodePosition POSITIONS_IN_DIRECTION[4][2] = {
        { NodePosition::POSITION_NW, NodePosition::POSITION_NE },
        { NodePosition::POSITION_SW, NodePosition::POSITION_SE },
        { NodePosition::POSITION_SE, NodePosition::POSITION_NE },
        { NodePosition::POSITION_SW, NodePosition::POSITION_NW }
    };

    struct TreeNode {
        int parent = -1;
        int depth = 0;
        BitField<LODMesh::LODMask> lod_mask = 0;
        int children[NodePosition::POSITION_MAX] = { -1, -1, -1, -1 };
        Rect2i rect;
        bool is_leaf() const {
            return children[0] == -1 && children[1] == -1 && children[2] == -1 && children[3] == -1;
        }
    };

    LocalVector<TreeNode> nodes;
    int tree_extent = 512;
    int max_depth = 5;
    float threshold_multiplier = Math_SQRT2;

private:
    struct NeighborData {
        int neighbor_idx = 0;
        NodeDirection direction;
    };
    int get_greater_or_equal_neighbor(const int p_node_idx, const NodeDirection p_direction) const;
    void get_neighbors(const int p_node_idx, LocalVector<NeighborData> &r_out) const;
    bool can_subdivide_node(const TreeNode &p_node) const;
    Vector2i get_position_in_parent(const TreeNode &p_parent, const NodePosition p_position) const;
    int get_depth_size(int p_depth) const;
    void subdivide_node(int p_idx);
    Vector2i calculate_node_center(const TreeNode &p_node) const;
    void _try_camera_subdiv(const Vector2i p_camera_pos, int p_node_idx);
    void constraint_lods(const int p_node_idx);
    LocalVector<int> get_local_neighbors(const int p_node_idx, const NodeDirection p_direction) const;
    TypedArray<Dictionary> get_leafs_bind() const;
public:
    static void _bind_methods();
    void clear();
    void insert_camera(const Vector2i p_camera_pos);
    static Ref<QuadTree> create(int p_extent, int p_max_depth, float p_threshold_multiplier);
    Rect2i get_node_rect_bind(int p_node_dix) const;
    PackedInt32Array get_node_neighbors_bind(int p_node_dix) const;


    struct LeafInformation {
        Rect2i rect;
        BitField<LODMesh::LODMask> lod_mask = 0;
        int depth = 0;
    };

    void get_leafs(LocalVector<LeafInformation> &r_out_leaves) const;
    QuadTree();
};
