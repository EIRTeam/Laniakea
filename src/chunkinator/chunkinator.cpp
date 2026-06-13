#include "chunkinator.h"
#include "chunkinator/chunkinator_bounds.h"
#include "chunkinator/chunkinator_debug_drawer.h"
#include "chunkinator/chunkinator_generator.h"
#include "chunkinator/chunkinator_layer.h"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/classes/worker_thread_pool.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include <optional>
#include <queue>

Ref<ChunkinatorLayer> Chunkinator::get_layer(StringName p_name) const {
    for (auto it : layers) {
        if (it->name == p_name) {
            return it;
        }
    }
    return nullptr;
}

LocalVector<Ref<ChunkinatorLayer>> Chunkinator::get_leaves() {
    LocalVector<Ref<ChunkinatorLayer>> leaves;
    for (Ref<ChunkinatorLayer> layer : layers) {
        if (layer->children.is_empty()) {
            leaves.push_back(layer);
        }
    }
    return leaves;
}

Vector2i get_chunk_indices(Vector2i p_pos, int p_chunk_size) {
    Vector2i chunk = Vector2i (
        p_pos.x / p_chunk_size,
        p_pos.y / p_chunk_size
    );

    // Handle negative positions
    if (p_pos.x < 0 && p_pos.x % p_chunk_size != 0) chunk.x--;
    if (p_pos.y < 0 && p_pos.y % p_chunk_size != 0) chunk.y--;

    return chunk;
}

ChunkinatorBounds Chunkinator::_world_rect_to_chunk_idx_bounds(Rect2i p_world_rect, int p_chunk_size, Vector2i p_padding) const {
    const Vector2i min_chunk = get_chunk_indices(p_world_rect.position - p_padding, p_chunk_size);
    const Vector2i max_chunk = get_chunk_indices(p_world_rect.position + p_world_rect.size + p_padding, p_chunk_size);

    return ChunkinatorBounds {
        .min_chunk = min_chunk,
        .max_chunk = max_chunk
    };
}

std::optional<Chunkinator::LayerDependency> Chunkinator::find_layer_dependency(StringName p_parent_name, StringName p_child_name) const {
    for (const LayerDependency &dep : layer_dependencies) {
        if (dep.parent == p_parent_name && dep.child == p_child_name) {
            return dep;
        }
    }
    return std::nullopt;
}

void Chunkinator::_recalculate_bounds(Ref<ChunkinatorLayer> p_node, Rect2i p_child_bounds, Vector2i p_child_padding) {

    // First calculate our own rect snapped to chunk boundaries
    const ChunkinatorBounds index_bounds = _world_rect_to_chunk_idx_bounds(p_child_bounds, p_node->get_chunk_size(), p_child_padding);

    p_node->chunk_idx_bounds_to_generate = index_bounds;
    p_node->world_rect_to_generate = Rect2i(index_bounds.min_chunk * p_node->get_chunk_size(), (index_bounds.max_chunk - index_bounds.min_chunk + Vector2i(1, 1)) * p_node->get_chunk_size());
    p_node->debug_world_rect_with_padding = Rect2i(p_child_bounds.position - p_child_padding, p_child_bounds.size + p_child_padding * 2);

    for (StringName parent : p_node->parents) {
        std::optional<LayerDependency> layer_dep = find_layer_dependency(parent, p_node->name);

        ERR_FAIL_COND_MSG(!layer_dep.has_value(), "Bug?");

        Ref<ChunkinatorLayer> parent_node = get_layer(parent);

        ERR_FAIL_COND_MSG(!parent_node.is_valid(), "Bug?");

        _recalculate_bounds(parent_node, p_node->world_rect_to_generate, layer_dep->padding);
    }
}

void Chunkinator::recalculate_bounds() {
    LocalVector<Ref<ChunkinatorLayer>> leaves = get_leaves();

    for (Ref<ChunkinatorLayer> leaf : leaves) {
        Ref<ChunkinatorGeneratorLayer> generator = leaf;
        ERR_FAIL_COND_MSG(!generator.is_valid(), "Leaf nodes must always be generators!");
        Rect2i leaf_rect = generator->get_generation_rect();
        _recalculate_bounds(leaf, leaf_rect, Vector2i());
    }
}

void Chunkinator::add_layer_dependency(StringName p_parent_name, StringName p_child_name, Vector2i p_padding) {
    Ref<ChunkinatorLayer> parent_layer = get_layer(p_parent_name);
    Ref<ChunkinatorLayer> child_layer = get_layer(p_child_name);
    ERR_FAIL_COND_MSG(parent_layer == nullptr, vformat("Parent layer with name %s not found!", p_parent_name));
    ERR_FAIL_COND_MSG(child_layer == nullptr, vformat("Child layer with name %s not found!", p_child_name));

    layer_dependencies.push_back({
        .padding = p_padding,
        .parent = p_parent_name,
        .child = p_child_name,
    });

    child_layer->parents.push_back(p_parent_name);
    parent_layer->children.push_back(p_child_name);
}

void Chunkinator::build() {
    LocalVector<int> in_degrees;

    in_degrees.reserve(layers.size());

    std::queue<int> processing_nodes;
    for (int i = 0; i < layers.size(); i++) {
        in_degrees.push_back(layers[i]->parents.size());

        if (layers[i]->parents.size() == 0) {
            processing_nodes.push(i);
        }
    }

    // Calculate DAG levels
    int processed_nodes = 0;
    for (; !processing_nodes.empty(); processing_nodes.pop()) {
        processed_nodes++;
        Ref<ChunkinatorLayer> current_layer = layers[processing_nodes.front()];
        for (StringName child : current_layer->children) {
            Ref<ChunkinatorLayer> child_layer = get_layer(child);
            int child_idx = layers.find(child_layer);
            child_layer->dag_level = Math::max(child_layer->dag_level, current_layer->dag_level+1);
            in_degrees[child_idx]--;
            if (in_degrees[child_idx] == 0) {
                processing_nodes.push(child_idx);
            }
        }
    }

    for (int i = 0; i < layers.size(); i++) {
        max_dag_level = Math::max(layers[i]->dag_level, max_dag_level);
    }

    for (Ref<ChunkinatorLayer> layer : layers) {
        print_line(vformat("Layer %s level %d", layer->name, layer->dag_level));
    }

    ERR_FAIL_COND_MSG(processed_nodes < layers.size(), "Graph contains a cycle!");
}

void Chunkinator::generate() {
    if (state == ChunkinatorGenerationState::GENERATING) {
        return;
    }
    generation_data = {};
    recalculate_bounds();
    generate_tasks();

    if (generation_data.task_count == 0) {
        return;
    }

    // Unload now unnecessary chunks

    for (int level = 0; level <= max_dag_level; level++) {
        for (int i = 0; i < layers.size(); i++) {
            Ref<ChunkinatorLayer> layer = layers[i];
            if (layer->dag_level != level) {
                continue;
            }
            const ChunkinatorBounds idx_bounds = layer->chunk_idx_bounds_to_generate;
            // Unload chunks not within our bounds
            for (int chunk_i = layer->chunks.size()-1; chunk_i >= 0; chunk_i--) {
                const Vector2i chunk_idx = layer->chunks[chunk_i]->get_chunk_index();
                if (!idx_bounds.is_chunk_in_bounds(chunk_idx)) {
                    if (generation_data.debug_snapshot.is_valid()) {
                        generation_data.debug_snapshot->per_layer_debug_data[i].deleted_chunks.push_back(layer->chunks[chunk_i]->get_chunk_index());
                        generation_data.debug_snapshot->per_layer_debug_data[i].chunks.push_back(layer->chunks[chunk_i]->get_chunk_index());
                    }
                    layer->chunks.remove_at(chunk_i);
                }
            }
        }
    }

    state = ChunkinatorGenerationState::GENERATING;
    if (capture_debug_snapshot) {
        generation_data.debug_snapshot.instantiate();
        generation_data.debug_snapshot->per_layer_debug_data.reserve(layers.size());
        for (Ref<ChunkinatorLayer> layer : layers) {
            generation_data.debug_snapshot->per_layer_debug_data.push_back({
                .layer = layer
            });
        }
    }


    start_task_for_level(0);
}

void Chunkinator::_generation_task(void *p_userdata, uint32_t p_idx) {
    ChunkinatorTask *task = (ChunkinatorTask*)p_userdata;
    Time *t = Time::get_singleton();
    int64_t start = t->get_ticks_usec();
    task->chunks[p_idx]->generate();
    int64_t end = t->get_ticks_usec();
    task->chunks[p_idx]->generation_duration = end - start;
}

void Chunkinator::insert_layer(StringName p_layer_name, Ref<ChunkinatorLayer> p_layer) {
    p_layer->name = p_layer_name;
    p_layer->chunkinator = this;
    p_layer->initialize_internal();
    layers.push_back(p_layer);
}

void Chunkinator::start_task_for_level(int p_level) {
    generation_data.current_level = p_level;
    LocalVector<ChunkinatorTask> &tasks_per_level = generation_data.tasks_per_level[p_level];

    if (tasks_per_level.is_empty()) {
        process_generation();
        return;
    }

    WorkerThreadPool *wtp = WorkerThreadPool::get_singleton();

    for (int i = 0; i < tasks_per_level.size(); i++) {
        tasks_per_level[i].task_id = wtp->add_native_group_task(&Chunkinator::_generation_task, &tasks_per_level.ptr()[i], tasks_per_level[i].chunks.size());
        print_line(vformat("Dispatched task for layer %s!", tasks_per_level[i].layer->name));
    }

}

void Chunkinator::process_generation() {
    if (state != ChunkinatorGenerationState::GENERATING) {
        return;
    }

    bool all_done = true;
    WorkerThreadPool *wtp = WorkerThreadPool::get_singleton();
    LocalVector<ChunkinatorTask> &tasks_for_level = generation_data.tasks_per_level[generation_data.current_level];
    for (int i = 0; i < tasks_for_level.size(); i++) {
        if (tasks_for_level[i].task_state == ChunkinatorTask::COMPLETED) {
            continue;
        }
        const bool task_completed = wtp->is_group_task_completed(tasks_for_level[i].task_id);

        if (!task_completed) {
            all_done = false;
            continue;
        }

        // Task is done!

        for (Ref<ChunkinatorChunk> chunk : tasks_for_level[i].chunks) {
            tasks_for_level[i].layer->chunks.push_back(chunk);
        }

        wtp->wait_for_group_task_completion(tasks_for_level[i].task_id);
        tasks_for_level[i].task_state = ChunkinatorTask::COMPLETED;

        // debug stuff
        if (!capture_debug_snapshot || !generation_data.debug_snapshot.is_valid()) {
            continue;
        }

        const int layer_idx = layers.find(tasks_for_level[i].layer);
        ChunkinatorLayerDebugSnapshot &debug_data = generation_data.debug_snapshot->per_layer_debug_data[layer_idx];
        debug_data.layer = tasks_for_level[i].layer;
        debug_data.generation_rect_with_padding = tasks_for_level[i].layer->debug_world_rect_with_padding;
        Time *t = Time::get_singleton();

        debug_data.chunks.reserve(tasks_for_level[i].layer->chunks.size());
        for (Ref<ChunkinatorChunk> chunk : tasks_for_level[i].layer->chunks) {
            debug_data.chunks.push_back(chunk->get_chunk_index());
            chunk->debug_draw(&debug_data.drawer);
        }

        debug_data.newly_generated_chunks.reserve(tasks_for_level[i].chunks.size());
        for (Ref<ChunkinatorChunk> chunk : tasks_for_level[i].chunks) {
            debug_data.newly_generated_chunks.push_back(chunk->get_chunk_index());
            debug_data.average_generation_time_usec += chunk->generation_duration;
        }

        debug_data.average_generation_time_usec /= tasks_for_level[i].chunks.size();

        print_line("Debug Data OK!");
    }

    // Check if we can move to the next level
    if (!all_done) {
        return;
    }


    if (generation_data.current_level == max_dag_level) {
        state = ChunkinatorGenerationState::IDLING;
        if (capture_debug_snapshot && generation_data.debug_snapshot.is_valid()) {
            generation_data.debug_snapshot->generation_rect = generation_data.generation_rect;
            emit_signal("debug_snapshot_created", generation_data.debug_snapshot);
        }
        emit_signal("generation_completed");
        return;
    }

    print_line(vformat("Level completed, moving to level %d", generation_data.current_level+1));

    start_task_for_level(generation_data.current_level+1);
}

void Chunkinator::set_generation_rect(Rect2i p_generation_rect) {
    generation_rect = p_generation_rect;
}

void Chunkinator::generate_tasks() {
    generation_data.task_count = 0;
    generation_data.tasks_per_level.clear();
    generation_data.tasks_per_level.resize(max_dag_level+1);
    for (int level = 0; level <= max_dag_level; level++) {
        for (int i = 0; i < layers.size(); i++) {
            Ref<ChunkinatorLayer> layer = layers[i];
            if (layer->dag_level != level) {
                continue;
            }
            const ChunkinatorBounds idx_bounds = layer->chunk_idx_bounds_to_generate;
            /*// Unload chunks not within our bounds
            for (int chunk_i = layer->chunks.size()-1; chunk_i >= 0; chunk_i--) {
                const Vector2i chunk_idx = layer->chunks[chunk_i]->get_chunk_index();
                if (!idx_bounds.is_chunk_in_bounds(chunk_idx)) {
                    if (generation_data.debug_snapshot.is_valid()) {
                        generation_data.debug_snapshot->per_layer_debug_data[i].deleted_chunks.push_back(layer->chunks[chunk_i]->get_chunk_index());
                        generation_data.debug_snapshot->per_layer_debug_data[i].chunks.push_back(layer->chunks[chunk_i]->get_chunk_index());
                        Time *t = Time::get_singleton();
                    }
                    layer->chunks.remove_at(chunk_i);
                }
            }*/

            // Instantiate new chunks
            ChunkinatorTask task = {
                .layer = layer
            };
            const int chunk_size = layer->get_chunk_size();
            for (int x = idx_bounds.min_chunk.x; x <= idx_bounds.max_chunk.x; x++) {
                for (int y = idx_bounds.min_chunk.y; y <= idx_bounds.max_chunk.y; y++) {
                    if (layer->get_chunk_by_index(Vector2i(x, y)).is_valid()) {
                        continue;
                    }
                    Ref<ChunkinatorChunk> chunk = layer->instantiate_chunk();
                    chunk->chunk_idx = Vector2(x, y);
                    chunk->world_bounds = Rect2i(chunk->chunk_idx * chunk_size, Vector2i(chunk_size, chunk_size));
                    task.chunks.push_back(chunk);
                    layer->chunks.push_back(chunk);
                }
            }

            // Dispatch the tasks
            if (task.chunks.is_empty()) {
                continue;
            }

            generation_data.tasks_per_level[level].push_back(task);
            generation_data.task_count++;
        }
    }

}

Chunkinator::ChunkinatorGenerationState Chunkinator::get_generation_state() const {
    return state;
}

void Chunkinator::_bind_methods() {
    ADD_SIGNAL(MethodInfo("debug_snapshot_created", PropertyInfo(Variant::OBJECT, "debug_snapshot", PROPERTY_HINT_RESOURCE_TYPE, "ChunkinatorDebugSnapshot")));
    ADD_SIGNAL(MethodInfo("generation_completed"));
}

LocalVector<ChunkinatorLayerDebugSnapshot> ChunkinatorDebugSnapshot::get_per_layer_debug_data() const {
    return per_layer_debug_data;
}

Rect2i ChunkinatorDebugSnapshot::get_generation_rect() const {
    return generation_rect;
}
