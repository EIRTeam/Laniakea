#pragma once

#include "../console/gui/console_gui.h"
#include "console/console_system.h"
#include "debug/debug_overlay.h"
#include "game/game_rules_laniakea.h"
#include "godot_cpp/classes/scene_tree.hpp"

using namespace godot;

class LaniakeaMainLoop : public SceneTree {
	GDCLASS(LaniakeaMainLoop, SceneTree);

	static CVar quit_command;
	static CVar timescale_cvar;
	static CVar command_echo;
	double physics_time = 0.0f;
	double process_time = 0.0f;

	ConsoleSystem *console_system;
	ConsoleGUI *console_gui;
	DebugOverlay *debug_overlay;
	LaniakeaGameRules *game_rules;

	static LaniakeaMainLoop *singleton;

public:
	void _on_timescale_cvar_changed();
	static void _bind_methods();
	virtual void _initialize() override;
	virtual bool _process(double p_delta) override;
	virtual bool _physics_process(double p_delta) override;
	virtual void _finalize() override;

	double get_physics_time() const;
	double get_process_time() const;
	void args_test(const String &p_echo);

	static LaniakeaMainLoop *get_singleton();
	LaniakeaGameRules *get_game_rules() const;
};
