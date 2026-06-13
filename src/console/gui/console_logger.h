#pragma once

#include "godot_cpp/classes/logger.hpp"
#include "godot_cpp/classes/script_backtrace.hpp"

using namespace godot;

class ConsoleLogger : public Logger {
	GDCLASS(ConsoleLogger, Logger);

public:
	static void _bind_methods();
	virtual void _log_error(const String &p_function, const String &p_file, int32_t p_line, const String &p_code, const String &p_rationale, bool p_editor_notify, int32_t p_error_type, const TypedArray<Ref<ScriptBacktrace>> &p_script_backtraces) override;
	virtual void _log_message(const String &p_message, bool p_error) override;
};
