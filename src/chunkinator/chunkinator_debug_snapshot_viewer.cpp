#include "chunkinator_debug_snapshot_viewer.h"
#include "chunkinator/chunkinator.h"
#include "chunkinator/chunkinator_debug_drawer.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/h_split_container.hpp"
#include "godot_cpp/classes/input_event_mouse_button.hpp"
#include "godot_cpp/classes/input_event_mouse_motion.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/variant/transform2d.hpp"

String prettify_interval(int64_t p_interval_usec) {
    if  (p_interval_usec > 1000) {
        return vformat("%.2f ms", p_interval_usec / (double)1000);
    }

    if (p_interval_usec > 1000000) {
        return vformat("%.2f s", p_interval_usec / (double)1000000);
    }

    return vformat("%d us", p_interval_usec);
}

void ChunkinatorDebugSnapshotViewer::_populate_layer_list() {
    layer_list->clear();

    LocalVector<ChunkinatorLayerDebugSnapshot> layers = debug_snapshot->get_per_layer_debug_data();

    for (int i = 0; i < layers.size(); i++) {
        layer_list->add_item(vformat("%s \nAvg: %s", layers[i].layer->get_name(), prettify_interval(layers[i].average_generation_time_usec)));
    }
}

ChunkinatorDebugSnapshotViewer::ChunkinatorDebugSnapshotViewer(const Ref<ChunkinatorDebugSnapshot> &p_debug_snapshot) {
    debug_snapshot = p_debug_snapshot;

    HBoxContainer *split = memnew(HBoxContainer);
    add_child(split);
    layer_viewer = memnew(ChunkinatorDebugLayerViewer);
    split->add_child(layer_viewer);
    layer_viewer->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    layer_list = memnew(ItemList);
    split->add_child(layer_list);
    layer_list->set_auto_width(true);
    layer_list->set_h_size_flags(Control::SIZE_SHRINK_END);

    layer_list->set_select_mode(ItemList::SELECT_SINGLE);

    layer_list->connect("item_selected", callable_mp(this, &ChunkinatorDebugSnapshotViewer::_on_layer_selected));

    _populate_layer_list();
}

void ChunkinatorDebugSnapshotViewer::_on_layer_selected(int p_idx) {
    layer_viewer->set_layer_snapshot(debug_snapshot->get_per_layer_debug_data()[p_idx]);
    layer_viewer->set_generation_rect(debug_snapshot->get_generation_rect());
}

Transform2D ChunkinatorDebugLayerViewer::get_draw_trf() const {
    if (draw_trf_dirty) {
        Transform2D draw_trf;
        Size2 size = get_size();
        float scale = size[size.min_axis_index()] / view_radius;
        draw_trf.set_origin(size * 0.5 + offset * scale);
        draw_trf.set_scale(Vector2(scale, scale));

        const_cast<ChunkinatorDebugLayerViewer*>(this)->draw_trf_cache = draw_trf;
        const_cast<ChunkinatorDebugLayerViewer*>(this)->draw_trf_dirty = false;
    }
    return draw_trf_cache;
}

void ChunkinatorDebugLayerViewer::_draw_debug_element(const ChunkinatorDebugDrawer::DrawTextureRect &p_draw_command) {
    Transform2D draw_trf = get_draw_trf();
    draw_texture_rect(p_draw_command.texture, draw_trf.xform(p_draw_command.world_draw_rect), false);
}

void ChunkinatorDebugLayerViewer::_draw_debug_element(const ChunkinatorDebugDrawer::DrawCircle &p_draw_command) {
    Transform2D draw_trf = get_draw_trf();
    draw_circle(draw_trf.xform(p_draw_command.position), p_draw_command.pixel_radius, p_draw_command.color);
}

void ChunkinatorDebugLayerViewer::_draw_debug_element(const ChunkinatorDebugDrawer::DrawLine &p_draw_command) {
    Transform2D draw_trf = get_draw_trf();
    draw_line(draw_trf.xform(p_draw_command.from), draw_trf.xform(p_draw_command.to), p_draw_command.color, p_draw_command.thickness);
}

void ChunkinatorDebugLayerViewer::_draw_debug_element(const ChunkinatorDebugDrawer::DrawRect &p_draw_command) {
    Transform2D draw_trf = get_draw_trf();
    draw_rect(draw_trf.xform(p_draw_command.rect), p_draw_command.color, p_draw_command.fill, p_draw_command.thickness);
}

void ChunkinatorDebugLayerViewer::set_layer_snapshot(ChunkinatorLayerDebugSnapshot p_debug_snapshot) {
    layer_snapshot = p_debug_snapshot;
    has_snapshot = true;
    queue_redraw();
}

void ChunkinatorDebugLayerViewer::set_generation_rect(Rect2i p_generation_rect) {
    generation_rect = p_generation_rect;
    queue_redraw();
}

void ChunkinatorDebugLayerViewer::_notification(int p_what) {
    switch(p_what) {
        case NOTIFICATION_RESIZED: {
            draw_trf_dirty = true;
        } break;
        case NOTIFICATION_DRAW: {
            if (!has_snapshot) {
                return;
            }

            const Color deleted_chunk_color_border = Color(1.0, 0.0, 0.0);
            const Color deleted_chunk_color_fill = Color(1.0, 0.0, 0.0, 0.25);

            const Color old_chunk_color_border = Color(0.75, 0.75, 0.75);
            const Color old_chunk_color_fill = Color(0.75, 0.75, 0.75, 0.25);

            const Color new_chunk_color_border = Color(0.0, 1.0, 0.0);
            const Color new_chunk_color_fill = Color(0.0, 1.0, 0.0, 0.25);

            Transform2D draw_trf = get_draw_trf();

            for (Vector2i chunk : layer_snapshot.chunks) {
                Color chunk_color_fill = old_chunk_color_fill;

                if (layer_snapshot.newly_generated_chunks.has(chunk)) {
                    chunk_color_fill = new_chunk_color_fill;
                } else if (layer_snapshot.deleted_chunks.has(chunk)) {
                    chunk_color_fill = deleted_chunk_color_fill;
                }

                const Rect2 chunk_rect = Rect2(Vector2(chunk) * layer_snapshot.layer->get_chunk_size(), Vector2(1.0, 1.0) * layer_snapshot.layer->get_chunk_size());
                //draw_rect(draw_trf.xform(chunk_rect), chunk_color_fill);
            }
            // Debug stuff must be drawn underneath it all
            for (const ChunkinatorDebugDrawer::DrawCommand &dc : layer_snapshot.drawer.get_draw_commands()) {
                std::visit([&](auto i_draw_command) {
                    return _draw_debug_element(i_draw_command);
                }, dc);
            }

            for (Vector2i chunk : layer_snapshot.chunks) {

                Color chunk_color_border = old_chunk_color_border;

                if (layer_snapshot.newly_generated_chunks.has(chunk)) {
                    chunk_color_border = new_chunk_color_border;
                } else if (layer_snapshot.deleted_chunks.has(chunk)) {
                    chunk_color_border = deleted_chunk_color_border;
                }

                const Rect2 chunk_rect = Rect2(Vector2(chunk) * layer_snapshot.layer->get_chunk_size(), Vector2(1.0, 1.0) * layer_snapshot.layer->get_chunk_size());
                draw_rect(draw_trf.xform(chunk_rect), chunk_color_border, false);
            }

            if (generation_rect != layer_snapshot.generation_rect_with_padding) {
                draw_rect(draw_trf.xform(layer_snapshot.generation_rect_with_padding), Color(1.0, 1.0, 0.0), false);
            }
            draw_rect(draw_trf.xform(generation_rect), Color(1.0, 0.0, 1.0), false);
            draw_circle(draw_trf.xform(generation_rect.get_center()), draw_trf.get_scale().x * 15000, Color(1.0, 0.0, 0.0), false);
        } break;
    }
}

void ChunkinatorDebugLayerViewer::_gui_input(const Ref<InputEvent> &p_event) {
    if (Ref<InputEventMouseButton> ev = p_event; ev.is_valid()) {
        if (!ev->is_pressed()) {
            return;
        }

        if (ev->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN) {
            view_radius += 1000.0f;
            draw_trf_dirty = true;
            queue_redraw();
        } else if (ev->get_button_index() == MOUSE_BUTTON_WHEEL_UP) {
            view_radius -= 1000.0f;
            draw_trf_dirty = true;
            queue_redraw();
        }
    }
    if (Ref<InputEventMouseMotion> ev = p_event; ev.is_valid()) {
        if (!Input::get_singleton()->is_mouse_button_pressed(MOUSE_BUTTON_MIDDLE)) {
            return;
        }
        const Transform2D draw_trf = get_draw_trf();
        Vector2 relative = ev->get_screen_relative();
        offset += draw_trf.affine_inverse().basis_xform(relative);
        draw_trf_dirty = true;
        queue_redraw();
    }
}

void ChunkinatorDebugLayerViewer::_bind_methods() {

}

ChunkinatorDebugLayerViewer::ChunkinatorDebugLayerViewer() {
    set_clip_contents(true);
}
