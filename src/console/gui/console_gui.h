#pragma once

#include "console/gui/console_logger.h"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/panel_container.hpp"

using namespace godot;

namespace godot {
class LineEdit;
class RichTextLabel;
class ItemList;
} //namespace godot

class CVar;

class ConsoleGUI : public Control {
	GDCLASS(ConsoleGUI, Control);
	void _notification(int p_what);

	void hide_console();
	void show_console();
	static void _bind_methods();

	RichTextLabel *text_label = nullptr;
	LineEdit *line_edit = nullptr;

	ItemList *autocomplete_item_list = nullptr;
	bool prev_pause_state = false;
	int current_history_index = -1;
	void _show_autocomplete(const String &p_text);
	void _autocomplete_accept();
	void _on_log_bbcode(const String &p_text);
	void _on_command_submitted(const String &p_command);
	Ref<ConsoleLogger> logger;
	Input::MouseMode prev_mouse_mode = Input::MOUSE_MODE_VISIBLE;
	String _get_command_preview(CVar *p_cvar) const;
	void recall_history(int p_step);

public:
	virtual void _shortcut_input(const Ref<InputEvent> &p_event) override;
	virtual void _input(const Ref<InputEvent> &p_event) override;

	ConsoleGUI();
	~ConsoleGUI();
};
