#pragma once

#include "godot_cpp/classes/texture2d.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/variant/rect2.hpp"

#include <variant>

using namespace godot;

class ChunkinatorDebugDrawer {
public:
	struct DrawTextureRect {
		Ref<Texture2D> texture;
		Rect2 world_draw_rect;
	};

	struct DrawCircle {
		float pixel_radius = 10.0f;
		Color color;
		Vector2 position;
	};

	struct DrawLine {
		Vector2 from;
		Vector2 to;
		Color color;
		float thickness;
	};

	struct DrawRect {
		Rect2 rect;
		Color color;
		float thickness;
		bool fill;
	};

	using DrawCommand = std::variant<
			DrawTextureRect,
			DrawCircle,
			DrawLine,
			DrawRect>;

private:
	LocalVector<DrawCommand> draw_commands;

public:
	void draw_texture(Ref<Texture2D> p_texture, Rect2 p_rect);
	void draw_circle(Vector2 p_position, float p_pixel_radius = 10.0f, Color p_color = Color(1.0, 0.0, 0.0));
	void draw_line(Vector2 p_from, Vector2 p_to, float p_thickness = -1.0f, Color p_color = Color(1.0, 0.0, 0.0));
	void draw_rect(Rect2 p_rect, float p_thickness = -1.0f, Color p_color = Color(1.0, 0.0, 0.0), bool p_fill = false);
	const LocalVector<DrawCommand> &get_draw_commands();
};
