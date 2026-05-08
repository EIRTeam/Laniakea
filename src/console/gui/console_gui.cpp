#include "console_gui.h"
#include "console/console_system.h"
#include "console/cvar.h"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/item_list.hpp"
#include "godot_cpp/classes/line_edit.hpp"
#include "godot_cpp/classes/panel_container.hpp"
#include "godot_cpp/classes/rich_text_label.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "godot_cpp/classes/rich_text_label.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/core/error_macros.hpp"

void ConsoleGUI::_shortcut_input(const Ref<InputEvent> &p_event) {
    if (p_event->is_action_pressed("toggle_console")) {
        if (!is_visible()) {
            show_console();
        } else {
            hide_console();
        }

        accept_event();
    }
}

void ConsoleGUI::_input(const Ref<InputEvent> &p_event) {
    if (line_edit->has_focus() && !autocomplete_item_list->is_visible()) {
        if (p_event->is_action_pressed("ui_up")) {
            recall_history(1);
            accept_event();
        }

        if (p_event->is_action_pressed("ui_down")) {
            recall_history(-1);
            accept_event();
        }
    }

    if (!autocomplete_item_list->is_visible()) {
        return;
    }

    // Autocomplete with tab (no need to select anything)
    if (p_event->is_action_pressed("ui_text_indent")) {
        _autocomplete_accept();
        accept_event();
        return;
    }

    if (p_event->is_action_pressed("ui_accept") && !autocomplete_item_list->get_selected_items().is_empty()) {
        _autocomplete_accept();
        accept_event();
        return;
    }

    const bool is_up_action = p_event->is_action_pressed("ui_up", true);
    const bool is_down_action = p_event->is_action_pressed("ui_down", true);

    if (is_up_action || is_down_action) {
        // Autocomplete move
        const int selected = autocomplete_item_list->get_selected_items().is_empty() ? -1 : autocomplete_item_list->get_selected_items()[0];
        int new_selected = selected;

        if (!autocomplete_item_list->get_selected_items().is_empty()) {
            if (is_down_action) {
                new_selected++;
            } else if (is_up_action) {
                new_selected--;
            }
        } else {
            new_selected = 0;
        }

        new_selected = CLAMP(new_selected, 0, autocomplete_item_list->get_item_count()-1);

        if (new_selected != selected) {
            autocomplete_item_list->select(new_selected);
            accept_event();
        }
    }
}

void ConsoleGUI::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
        } break;
        case NOTIFICATION_EXIT_TREE: {

        } break;
    }
}

void ConsoleGUI::hide_console() {
    hide();
    Input::get_singleton()->set_mouse_mode(prev_mouse_mode);
    get_tree()->set_pause(prev_pause_state);
}

void ConsoleGUI::show_console() {
    show();
    line_edit->grab_focus();

    prev_mouse_mode = Input::get_singleton()->get_mouse_mode();
    Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
    prev_pause_state = get_tree()->is_paused();
    get_tree()->set_pause(true);
}

void ConsoleGUI::_bind_methods() {
    
}

void ConsoleGUI::_show_autocomplete(const String &p_text) {
    autocomplete_item_list->hide();

    if (p_text.is_empty()) {
        return;
    }

    Vector<ConsoleSystem::CVarAutocompleteResult> results = ConsoleSystem::get_singleton()->do_autocomplete(p_text.strip_edges());
    
    if (results.is_empty()) {
        return;
    }

    if (line_edit->get_text().begins_with(results[0].cvar->get_cvar_name_string()) && !autocomplete_item_list->is_visible()) {
        return;
    }

    autocomplete_item_list->show();
    autocomplete_item_list->set_global_position(line_edit->get_global_position() + Vector2(0.0f, line_edit->get_size().y));

    autocomplete_item_list->clear();

    for (const ConsoleSystem::CVarAutocompleteResult &result : results) {
        int32_t item_idx;

        if (result.cvar->is_command()) {
            item_idx = autocomplete_item_list->add_item(_get_command_preview(result.cvar));
        } else {
            item_idx = autocomplete_item_list->add_item(result.cvar->get_cvar_name_string() + " " + result.cvar->get_value_display_string());
        }
        autocomplete_item_list->set_item_metadata(item_idx, result.cvar->get_cvar_name());
    }
}

void ConsoleGUI::_autocomplete_accept() {
    const int selected = autocomplete_item_list->get_selected_items().is_empty() ? 0 : autocomplete_item_list->get_selected_items()[0];
    autocomplete_item_list->hide();
    StringName cvar_name = autocomplete_item_list->get_item_metadata(selected);
    CVar *cvar = ConsoleSystem::get_singleton()->get_cvar(cvar_name);
    DEV_ASSERT(cvar != nullptr);

    if (cvar->is_command()) {
        line_edit->set_text( cvar->get_cvar_name_string() + " ");
    } else {
        line_edit->set_text(cvar->get_cvar_name_string() + " " + cvar->get_value_display_string());
    }
    line_edit->set_caret_column(line_edit->get_text().length());
    
}

void ConsoleGUI::_on_log_bbcode(const String &p_text) {
    text_label->append_text(p_text);
}

void ConsoleGUI::_on_command_submitted(const String &p_command) {
    ConsoleSystem::get_singleton()->execute_user_command(line_edit->get_text());
    line_edit->clear();
    current_history_index = -1;
    ConsoleSystem::get_singleton()->push_history(p_command);
}

String ConsoleGUI::_get_command_preview(CVar *p_cvar) const {
    DEV_ASSERT(p_cvar->is_command());
    
    const Vector<PropertyInfo> &property_infos = p_cvar->get_command_arguments();

    if (property_infos.is_empty()) {
        return p_cvar->get_cvar_name_string();
    }

    PackedStringArray preview_string_parts;
    preview_string_parts.resize(property_infos.size()+1);
    String *preview_str_parts_w = preview_string_parts.ptrw();
    preview_str_parts_w[0] = p_cvar->get_cvar_name_string();
    for (int i = 0; i < property_infos.size(); i++) {
        preview_str_parts_w[i+1] = "<" + property_infos[i].name + ">";
    }

    return String(" ").join(preview_string_parts);
}

void ConsoleGUI::recall_history(int p_step) {
    const int prev_index = current_history_index;
    const int entry_count = ConsoleSystem::get_singleton()->get_history_entry_count();
    current_history_index += p_step;
    current_history_index = CLAMP(p_step, -1, entry_count-1);

    String history_line;

    if (current_history_index >= 0) {
        history_line = ConsoleSystem::get_singleton()->recall_history(current_history_index);
    }

    line_edit->set_text(history_line);
    line_edit->set_caret_column(line_edit->get_text().length());
}

ConsoleGUI::ConsoleGUI() {
    set_anchors_and_offsets_preset(PRESET_FULL_RECT);

    PanelContainer *console_container = memnew(PanelContainer);
    add_child(console_container);
    console_container->set_anchor(SIDE_LEFT, 0.0);
    console_container->set_anchor(SIDE_TOP, 0.0);
    console_container->set_anchor(SIDE_RIGHT, 1.0);
    console_container->set_anchor(SIDE_BOTTOM, 0.5);

    text_label = memnew(RichTextLabel);
    text_label->set_v_size_flags(SIZE_EXPAND_FILL);
    text_label->set_h_size_flags(SIZE_EXPAND_FILL);
    text_label->set_scroll_follow(true);
    VBoxContainer *vbox = memnew(VBoxContainer);
    console_container->add_child(vbox);
    vbox->set_anchors_and_offsets_preset(LayoutPreset::PRESET_FULL_RECT);
    vbox->add_child(text_label);
    
    line_edit = memnew(LineEdit);
    line_edit->set_keep_editing_on_text_submit(true);
    line_edit->set_h_size_flags(SIZE_EXPAND_FILL);
    vbox->add_child(line_edit);
    line_edit->connect("text_changed", callable_mp(this, &ConsoleGUI::_show_autocomplete));
    line_edit->connect("text_submitted", callable_mp(this, &ConsoleGUI::_on_command_submitted));

    autocomplete_item_list = memnew(ItemList);
    add_child(autocomplete_item_list);
    autocomplete_item_list->set_auto_width(true);
    autocomplete_item_list->set_custom_minimum_size(Vector2(0, 256));
    autocomplete_item_list->hide();

    logger.instantiate();

    OS::get_singleton()->add_logger(logger);

    logger->connect("log_bbcode", callable_mp(this, &ConsoleGUI::_on_log_bbcode));

    set_process_mode(PROCESS_MODE_ALWAYS);

    hide();
}

ConsoleGUI::~ConsoleGUI() {
    OS::get_singleton()->remove_logger(logger);
}
