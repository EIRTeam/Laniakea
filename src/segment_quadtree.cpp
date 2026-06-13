#include "segment_quadtree.h"

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/random_number_generator.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "godot_cpp/variant/transform2d.hpp"
#include "profiling.h"

#include <limits>
#include <stack>
Rect2i SegmentQuadtree::calculate_rect_for_pos(const Rect2i &p_parent_rect, NodePosition p_pos) {
	const int halfwidth = p_parent_rect.size.x / 2;
	switch (p_pos) {
		case POSITION_NW: {
			return Rect2i(p_parent_rect.position, Vector2i(halfwidth, halfwidth));
		} break;
		case POSITION_NE: {
			return Rect2i(p_parent_rect.position + Vector2i(halfwidth, 0), Vector2i(halfwidth, halfwidth));
		} break;
		case POSITION_SW: {
			return Rect2i(p_parent_rect.position + Vector2i(0, halfwidth), Vector2i(halfwidth, halfwidth));
		} break;
		case POSITION_SE: {
			return Rect2i(p_parent_rect.position + Vector2i(halfwidth, halfwidth), Vector2i(halfwidth, halfwidth));
		} break;
		case POSITION_MAX: {
			ERR_FAIL_V_MSG(Rect2i(), "Should never happen");
		} break;
	}
	return Rect2i();
}
void SegmentQuadtree::_insert_child_segments(const int p_node_idx, const LocalVector<int> &p_segments) {
	QuadTreeNode &node = nodes[p_node_idx];
	const Rect2i &rect = node.rect;
	//const Rect2i child_rect = node.children_begin != -1 ? nodes[node.children_begin + p_pos].rect : calculate_rect_for_pos(node.rect, p_pos);

	LocalVector<int> intersecting_segments_idx;
	for (int i = 0; i < p_segments.size(); i++) {
		const QuadTreeSegment &segment = segments[p_segments[i]];
		if (Geometry::segment_intersects_rect(segment.start, segment.end, rect)) {
			intersecting_segments_idx.push_back(p_segments[i]);
		}
	}

	if (intersecting_segments_idx.is_empty()) {
		return;
	}

	node.indices_begin = node_segment_indices.size();
	node.indices_count = intersecting_segments_idx.size();

	node_segment_indices.resize(node_segment_indices.size() + intersecting_segments_idx.size());

	for (int i = 0; i < intersecting_segments_idx.size(); i++) {
		node_segment_indices[node.indices_begin + i] = intersecting_segments_idx[i];
	}

	if (node.depth >= TREE_MAX_DEPTH) {
		return;
	}

	if (node.children_begin == -1) {
		node.children_begin = nodes.size();
		nodes.resize(nodes.size() + 4);
		node = nodes[p_node_idx];
		for (int i = 0; i < NodePosition::POSITION_MAX; i++) {
			nodes[node.children_begin + i].rect = calculate_rect_for_pos(node.rect, (NodePosition)i);
			nodes[node.children_begin + i].depth = node.depth + 1;
			nodes[node.children_begin + i].parent = p_node_idx;
		}
	}

	int children_start = node.children_begin;
	for (int i = 0; i < NodePosition::POSITION_MAX; i++) {
		_insert_child_segments(children_start + i, intersecting_segments_idx);
	}
}
void SegmentQuadtree::initialize(Rect2i p_tree_world_rect, const LocalVector<QuadTreeSegment> &p_segments) {
	FuncProfile;
	segments = p_segments;
	tree_world_rect = p_tree_world_rect;
	nodes.push_back({ .rect = p_tree_world_rect });

	LocalVector<int> segment_indices;

	segment_indices.resize(p_segments.size());

	for (int i = 0; i < p_segments.size(); i++) {
		segment_indices[i] = i;
	}

	_insert_child_segments(0, segment_indices);
}
int SegmentQuadtree::find_closest_segment(const Vector2 &p_world_point, std::optional<float> p_max_radius, float *r_distance, Vector2 *r_closest_point) const {
	FuncProfile;

	const float max_radius_squared = p_max_radius.has_value() ? *p_max_radius * *p_max_radius : std::numeric_limits<float>::max();

	if (!tree_world_rect.has_point(p_world_point)) {
		return -1;
	}
	std::stack<int> node_queue;
	node_queue.push(0);

	while (nodes[node_queue.top()].children_begin != -1) {
		int current_node = node_queue.top();
		for (int i = 0; i < POSITION_MAX; i++) {
			if (nodes[nodes[current_node].children_begin + i].rect.has_point(p_world_point)) {
				node_queue.push(nodes[current_node].children_begin + i);
				break;
			}
		}
	}

	// We got a path from the root, now to backtrack and see what we can do...

	float closest_segment_distance_squared = std::numeric_limits<float>::max();
	int closest_segment_idx = -1;
	Vector2 closest_segment_point;

	HashSet<int> visited_nodes;
	HashSet<int> visited_segments;

	while (!node_queue.empty()) {
		int current_node = node_queue.top();
		node_queue.pop();

		const QuadTreeNode &node = nodes[current_node];

		// We are a leaf!
		if (node.children_begin == -1 && node.indices_begin != -1) {
			for (int i = 0; i < node.indices_count; i++) {
				const int segment_idx = node_segment_indices[node.indices_begin + i];
				if (visited_segments.has(segment_idx)) {
					continue;
				}
				const QuadTreeSegment &segment = segments[segment_idx];
				Vector2 point = Geometry::get_closest_point_to_segment(p_world_point, segment.start, segment.end);
				const float dist_sq_to_segment = p_world_point.distance_squared_to(point);
				if (dist_sq_to_segment < closest_segment_distance_squared) {
					closest_segment_distance_squared = dist_sq_to_segment;
					closest_segment_idx = segment_idx;
					closest_segment_point = point;
				}
				visited_segments.insert(segment_idx);
			}
		}

		if (visited_nodes.has(current_node)) {
			continue;
		}

		visited_nodes.insert(current_node);

		if (node.children_begin == -1) {
			continue;
		}

		// We are not a leaf!
		for (int i = 0; i < POSITION_MAX; i++) {
			int child_idx = node.children_begin + i;
			if (child_idx == current_node) {
				continue;
			}

			// Could the child possibly contain a closer segment?
			float dist_to_rect_squared = Geometry::distance_to_rect_squared(p_world_point, Rect2(nodes[child_idx].rect).get_center(), nodes[child_idx].rect.size.x);

			// Yes it could, we have to check it just in case.
			if (dist_to_rect_squared > max_radius_squared) {
				return -1;
			}
			if (dist_to_rect_squared < closest_segment_distance_squared) {
				node_queue.push(child_idx);
			}
		}
	}

	if (closest_segment_idx != -1) {
		if (r_distance != nullptr) {
			*r_distance = Math::sqrt(closest_segment_distance_squared);
		}

		if (r_closest_point != nullptr) {
			*r_closest_point = closest_segment_point;
		}
	}

	return closest_segment_idx;
}

SegmentQuadtree::QuadTreeSegment SegmentQuadtree::get_segment(int p_segment_idx) const {
	ERR_FAIL_INDEX_V(p_segment_idx, segments.size(), QuadTreeSegment {});
	return segments[p_segment_idx];
}

Transform2D SegmentQuadTreeDebug::get_draw_trf() const {
	Transform2D draw_trf;
	Size2 size = get_size();
	float scale = size[size.min_axis_index()] / (float)tree.tree_world_rect.size.x;
	float half_tree_size = tree.tree_world_rect.size.x * 0.5;
	Vector2 offset = -Vector2(half_tree_size, half_tree_size);
	draw_trf.set_origin(size * 0.5 + offset * scale);
	draw_trf.set_scale(Vector2(scale, scale));
	return draw_trf;
}

void SegmentQuadTreeDebug::_bind_methods() {
}

SegmentQuadTreeDebug::SegmentQuadTreeDebug() {
	LocalVector<SegmentQuadtree::QuadTreeSegment> segments;

	Ref<RandomNumberGenerator> rng;
	rng.instantiate();
	rng->set_seed(123123);

	const int TEST_RECT_SIZE = 4096;

	for (int i = 0; i < 16; i++) {
		segments.push_back(SegmentQuadtree::QuadTreeSegment {
				.start = Vector2(rng->randf_range(0.0, TEST_RECT_SIZE), rng->randf_range(0.0, TEST_RECT_SIZE)),
				.end = Vector2(rng->randf_range(0.0, TEST_RECT_SIZE), rng->randf_range(0.0, TEST_RECT_SIZE)),
		});
	}

	tree.initialize(Rect2i(0.0, 0.0, TEST_RECT_SIZE, TEST_RECT_SIZE), segments);
	set_process(true);
}

void SegmentQuadTreeDebug::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			Input *input = Input::get_singleton();
			if (input->is_mouse_button_pressed(godot::MOUSE_BUTTON_LEFT)) {
				Vector2 mouse_pos_quadtree = get_draw_trf().affine_inverse().xform(get_local_mouse_position());
				test_point = mouse_pos_quadtree.clamp(Vector2(), Vector2(tree.tree_world_rect.size.x, tree.tree_world_rect.size.x));
				queue_redraw();
			}
		} break;
		case NOTIFICATION_DRAW: {
			Transform2D draw_trf = get_draw_trf();

			for (int i = 0; i < tree.nodes.size(); i++) {
				draw_rect(draw_trf.xform(tree.nodes[i].rect), Color(0, 0, 0), false);
			}
			for (int i = 0; i < tree.segments.size(); i++) {
				draw_line(draw_trf.xform(tree.segments[i].start), draw_trf.xform(tree.segments[i].end), Color(1.0, 0.0, 0.0));
			}

			float out_dist = 0.0f;
			int closest_segment_idx = tree.find_closest_segment(test_point, std::nullopt, &out_dist);

			if (closest_segment_idx != -1) {
				draw_circle(draw_trf.xform(test_point), draw_trf.get_scale().x * out_dist, Color(0.0, 0.0, 1.0), false);
				Vector2 closest_point = Geometry::get_closest_point_to_segment(test_point, tree.segments[closest_segment_idx].start, tree.segments[closest_segment_idx].end);
				draw_circle(draw_trf.xform(closest_point), 5.0f, Color(1.0, 1.0, 0.0));
			}

			draw_circle(draw_trf.xform(test_point), 5.0, Color(0.0, 1.0, 0.0));
		} break;
	}
}
