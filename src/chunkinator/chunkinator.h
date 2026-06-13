#pragma once

#include "chunkinator/chunkinator_bounds.h"
#include "chunkinator/chunkinator_chunk.h"
#include "chunkinator/chunkinator_debug_drawer.h"
#include "chunkinator/chunkinator_layer.h"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/worker_thread_pool.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/safe_refcount.hpp"
#include "godot_cpp/variant/string_name.hpp"

#include <optional>

using namespace godot;

class Chunkinator;
class ChunkinatorDebugger;

struct ChunkinatorLayerDebugSnapshot {
	Ref<ChunkinatorLayer> layer;
	ChunkinatorDebugDrawer drawer;
	LocalVector<Vector2i> chunks;
	LocalVector<Vector2i> newly_generated_chunks;
	LocalVector<Vector2i> deleted_chunks;
	Rect2i generation_rect_with_padding;
	int64_t average_generation_time_usec = 0;
};

class ChunkinatorDebugSnapshot : public RefCounted {
	LocalVector<ChunkinatorLayerDebugSnapshot> per_layer_debug_data;
	Rect2i generation_rect;

public:
	LocalVector<ChunkinatorLayerDebugSnapshot> get_per_layer_debug_data() const;
	Rect2i get_generation_rect() const;
	friend class Chunkinator;
};

class Chunkinator : public RefCounted {
	GDCLASS(Chunkinator, RefCounted);
	struct LayerDependency {
		Vector2i padding;
		StringName parent;
		StringName child;
	};

public:
	enum ChunkinatorGenerationState {
		GENERATING,
		IDLING
	};

private:
	struct ChunkinatorTask {
		enum TaskState {
			WORKING,
			COMPLETED
		};
		TaskState task_state = TaskState::WORKING;
		WorkerThreadPool::GroupID task_id;
		Ref<ChunkinatorLayer> layer;
		LocalVector<Ref<ChunkinatorChunk>> chunks;
	};

	struct ChunkinatorGenerationData {
		LocalVector<LocalVector<ChunkinatorTask>> tasks_per_level;
		Ref<ChunkinatorDebugSnapshot> debug_snapshot;
		int current_level = 0;
		int task_count = 0;
		Rect2i generation_rect;
	};

	ChunkinatorGenerationData generation_data;
	int max_dag_level = 0;

	ChunkinatorGenerationState state = ChunkinatorGenerationState::IDLING;
	LocalVector<LayerDependency> layer_dependencies;
	LocalVector<Ref<ChunkinatorLayer>> layers;
	bool capture_debug_snapshot = true;

	LocalVector<Ref<ChunkinatorLayer>> get_leaves();
	ChunkinatorBounds _world_rect_to_chunk_idx_bounds(Rect2i p_rect, int p_chunk_size, Vector2i p_padding) const;
	Rect2i generation_rect;

	std::optional<LayerDependency> find_layer_dependency(StringName p_parent_name, StringName p_child_name) const;
	void _recalculate_bounds(Ref<ChunkinatorLayer> p_node, Rect2i p_child_bounds, Vector2i p_child_padding);
	void recalculate_bounds();
	static void _generation_task(void *p_userdata, uint32_t p_idx);

public:
	Ref<ChunkinatorLayer> get_layer(StringName p_name) const;
	void insert_layer(StringName p_layer_name, Ref<ChunkinatorLayer> p_layer);
	void add_layer_dependency(StringName p_parent_name, StringName p_child_name, Vector2i p_padding);
	void build();
	void generate();
	void start_task_for_level(int p_level);
	void finish_generation();
	void process_generation();
	void set_generation_rect(Rect2i p_generation_rect);
	void generate_tasks();
	ChunkinatorGenerationState get_generation_state() const;

	static void _bind_methods();

	friend class ChunkinatorDebugger;
};
