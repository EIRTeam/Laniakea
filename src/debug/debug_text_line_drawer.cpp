#include "debug_text_line_drawer.h"

#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/font.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/viewport.hpp"
void DebugTextLineDrawer::_bind_methods() {
}
void DebugTextLineDrawer::_draw() {
	Camera3D *cam = get_viewport()->get_camera_3d();
	if (cam == nullptr) {
		return;
	}

	for (const StringToDraw &string_to_draw : strings_to_draw) {
		if (cam->is_position_behind(string_to_draw.world_pos)) {
			continue;
		}

		const Ref<Font> font = get_theme_font("debug_text_font");
		const int font_size = get_theme_font_size("debug_text_size");
		const Vector2 draw_pos = cam->unproject_position(string_to_draw.world_pos);
		draw_multiline_string_outline(font, draw_pos, string_to_draw.text, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, -1, 5, Color(0.0, 0.0, 0.0));
		draw_multiline_string(font, draw_pos, string_to_draw.text, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
	}
}

void DebugTextLineDrawer::clear_strings() {
	strings_to_draw.clear();
	queue_redraw();
}

void DebugTextLineDrawer::add_string(const Vector3 &p_at, const String &p_text) {
	strings_to_draw.push_back({
			.world_pos = p_at,
			.text = p_text,
	});
	queue_redraw();
}
