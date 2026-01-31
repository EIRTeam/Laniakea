#include "main_loop.h"
#include "debug/debug_overlay.h"
#include "game/game_rules_laniakea.h"
#include "gdextension_interface.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/core/print_string.hpp"

LaniakeaMainLoop *LaniakeaMainLoop::singleton = nullptr;

CVar LaniakeaMainLoop::quit_command = CVar::create_command("quit", "Quits the game");
CVar LaniakeaMainLoop::timescale_cvar = CVar::create_variable("timescale", GDEXTENSION_VARIANT_TYPE_FLOAT, 1.0f, "Changes the game's speed", PROPERTY_HINT_NONE, "");
CVar LaniakeaMainLoop::command_echo = CVar::create_command("echo", "Prints back a given message.", { CVar::DelayedPropertyInfo(Variant::STRING, "message") });


void LaniakeaMainLoop::_on_timescale_cvar_changed() {
    Engine::get_singleton()->set_time_scale(timescale_cvar.get_float());
}

void LaniakeaMainLoop::_bind_methods() {
    
}

void LaniakeaMainLoop::_initialize() {
    console_system = memnew(ConsoleSystem);
    console_system->initialize();

    console_gui = memnew(ConsoleGUI);
    get_root()->add_child(console_gui);
    print_line_rich("[b]Project Laniakea[/b]");
    Dictionary version_info = Engine::get_singleton()->get_version_info();
    version_info["hash"] = String(version_info["hash"]).substr(0, 9);
    print_line(String("Shinobu Engine v{major}.{minor}.{status}.{build}.{hash} - https://eirteam.moe").format(version_info));

    quit_command.connect_command_callback(callable_mp(static_cast<SceneTree*>(this), &SceneTree::quit).bind(0));

    debug_overlay = memnew(DebugOverlay);
    debug_overlay->initialize(this);

    game_rules = memnew(LaniakeaGameRules);
    game_rules->initialize();
    
    timescale_cvar.connect_cvar_changed_callback(callable_mp(this, &LaniakeaMainLoop::_on_timescale_cvar_changed));

    singleton = this;

    command_echo.connect_command_callback(callable_mp(this, &LaniakeaMainLoop::args_test));
}

bool LaniakeaMainLoop::_process(double p_delta) {
    if (!is_paused()) {
        process_time += p_delta;
    }

    debug_overlay->advance(DebugOverlay::ProcessPass::PROCESS);
    return SceneTree::_process(p_delta);
}

bool LaniakeaMainLoop::_physics_process(double p_delta) {
    if (!is_paused()) {
        physics_time += p_delta;
    }

    debug_overlay->advance(DebugOverlay::ProcessPass::PHYSICS);
    return SceneTree::_physics_process(p_delta);
}

void LaniakeaMainLoop::_finalize() {
    memdelete(console_system);
    memdelete(debug_overlay);
    memdelete(game_rules);
}

double LaniakeaMainLoop::get_physics_time() const {
    return physics_time;
}

double LaniakeaMainLoop::get_process_time() const {
    return process_time;
}

void LaniakeaMainLoop::args_test(const String &p_echo) {
    print_line("Echoed: ", p_echo);
}

LaniakeaMainLoop *LaniakeaMainLoop::get_singleton() {
    return singleton;
}

LaniakeaGameRules *LaniakeaMainLoop::get_game_rules() const {
    return game_rules;
}
