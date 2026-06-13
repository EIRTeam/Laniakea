#pragma once

#include "godot_cpp/classes/control.hpp"

using namespace godot;

class DebugTextLineDrawer : public Control {
	GDCLASS(DebugTextLineDrawer, Control);

	struct StringToDraw {
		Vector3 world_pos;
		String text;
	};
	LocalVector<StringToDraw> strings_to_draw;

public:
	static void _bind_methods();
	void _draw() override;
	void clear_strings();
	void add_string(const Vector3 &p_at, const String &p_text);
};
