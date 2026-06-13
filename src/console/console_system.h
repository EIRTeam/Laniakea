#pragma once

#include "console/cvar.h"
#include "godot_cpp/classes/config_file.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"

using namespace godot;

class ConsoleSystem : public Object {
	GDCLASS(ConsoleSystem, Object);
	static CVar save_changed_vars;
	static CVar print_changed_command;

	Ref<ConfigFile> cvar_config_file;
	Ref<ConfigFile> history_config_file;
	HashMap<StringName, CVar *> registered_cvars;
	HashMap<StringName, Ref<CVarProxy>> cvar_proxies;
	PackedStringArray history_entries;
	static ConsoleSystem *singleton;

	Ref<CVarProxy> get_proxy(const StringName &p_cvar) const;
	void _update_cvar_autosave(CVar *p_cvar);
	void register_cvar(CVar *p_cvar);

public:
	static void _bind_methods() {}
	static ConsoleSystem *get_singleton();
	void initialize();

	CVar *get_cvar(StringName p_cvar_name);

	struct CVarAutocompleteResult {
		CVar *cvar;
		float similarity;
	};

	Vector<CVarAutocompleteResult> do_autocomplete(const String &p_text) const;
	void execute_user_command(const String &p_command_line);
	void print_changed_cvars();
	bool set_cvar(const StringName &p_cvar, const Variant &p_value, bool p_silent = false, bool p_show_error = true);

	void push_history(const String &p_command);
	String recall_history(int p_idx);
	int get_history_entry_count() const;

	ConsoleSystem();
	friend class CVar;
};
