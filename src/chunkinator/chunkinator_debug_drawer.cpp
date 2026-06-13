#include "chunkinator_debug_drawer.h"

void ChunkinatorDebugDrawer::draw_texture(Ref<Texture2D> p_texture, Rect2 p_rect) {
	draw_commands.push_back(DrawTextureRect {
			.texture = p_texture,
			.world_draw_rect = p_rect,
	});
}

void ChunkinatorDebugDrawer::draw_circle(Vector2 p_position, float p_pixel_radius, Color p_color) {
	draw_commands.push_back(DrawCircle {
			.pixel_radius = p_pixel_radius,
			.color = p_color,
			.position = p_position,
	});
}

void ChunkinatorDebugDrawer::draw_line(Vector2 p_from, Vector2 p_to, float p_thickness, Color p_color) {
	draw_commands.push_back(DrawLine {
			.from = p_from,
			.to = p_to,
			.color = p_color,
			.thickness = p_thickness,
	});
}

void ChunkinatorDebugDrawer::draw_rect(Rect2 p_rect, float p_thickness, Color p_color, bool p_fill) {
	draw_commands.push_back(DrawRect {
			.rect = p_rect,
			.color = p_color,
			.thickness = p_thickness,
			.fill = p_fill,
	});
}

const LocalVector<ChunkinatorDebugDrawer::DrawCommand> &ChunkinatorDebugDrawer::get_draw_commands() {
	return draw_commands;
}
