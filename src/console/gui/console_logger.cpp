#include "console_logger.h"

#include "godot_cpp/core/property_info.hpp"

String bbcode_from_ansi(const String &p_string) {
	String output;
	String pending_text;
	int pos = 0;

	// Track current active states to generate closing tags in reverse order later
	Vector<String> active_tags;

	while (pos < p_string.length()) {
		// Look for the start of an ANSI escape sequence
		int esc_pos = p_string.find("\u001b[", pos);
		if (esc_pos == -1) {
			// No more escapes - append remaining text and finish
			output += pending_text + p_string.substr(pos);
			break;
		}

		// Add any plain text before the escape
		if (esc_pos > pos) {
			pending_text += p_string.substr(pos, esc_pos - pos);
		}

		// Find the 'm' that terminates the escape sequence
		int m_pos = p_string.find("m", esc_pos + 2);
		if (m_pos == -1) {
			// Malformed - just append the rest as-is
			output += pending_text + p_string.substr(esc_pos);
			break;
		}

		// Flush pending text before processing the code
		output += pending_text;
		pending_text = "";

		String code_str = p_string.substr(esc_pos + 2, m_pos - (esc_pos + 2));
		PackedStringArray codes = code_str.split(";", false);

		// Process each numeric code in the sequence
		for (int i = 0; i < codes.size(); ++i) {
			int code = codes[i].to_int();

			if (code == 0) {
				// Reset everything - close all active tags in reverse order
				for (int j = active_tags.size() - 1; j >= 0; --j) {
					output += "[/" + active_tags[j] + "]";
				}
				active_tags.clear();
			} else if (code == 1) {
				output += "[b]";
				active_tags.append("b");
			} else if (code == 22) {
				if (active_tags.has("b")) {
					active_tags.erase("b");
					output += "[/b]";
				}
			} else if (code == 3) {
				output += "[i]";
				active_tags.append("i");
			} else if (code == 23) {
				if (active_tags.has("i")) {
					active_tags.erase("i");
					output += "[/i]";
				}
			} else if (code == 4) {
				output += "[u]";
				active_tags.append("u");
			} else if (code == 24) {
				if (active_tags.has("u")) {
					active_tags.erase("u");
					output += "[/u]";
				}
			} else if (code == 9) {
				output += "[s]";
				active_tags.append("s");
			} else if (code == 29) {
				if (active_tags.has("s")) {
					active_tags.erase("s");
					output += "[/s]";
				}
			} else if (code == 2) {
				output += "[code]";
				active_tags.append("code");
			}
			// Foreground colors (30-37, 90-97, 39 reset)
			else if (code >= 30 && code <= 37) {
				String color_name;
				switch (code) {
					case 30:
						color_name = "black";
						break;
					case 31:
						color_name = "red";
						break;
					case 32:
						color_name = "green";
						break;
					case 33:
						color_name = "yellow";
						break;
					case 34:
						color_name = "blue";
						break;
					case 35:
						color_name = "magenta";
						break;
					case 36:
						color_name = "cyan";
						break;
					case 37:
						color_name = "white";
						break;
				}
				output += "[color=" + color_name + "]";
				active_tags.append("color");
			} else if (code >= 90 && code <= 97) {
				String color_name;
				switch (code) {
					case 90:
						color_name = "gray";
						break;
					case 91:
						color_name = "red";
						break;
					case 92:
						color_name = "lime";
						break; // or "green"
					case 93:
						color_name = "yellow";
						break;
					case 94:
						color_name = "blue";
						break;
					case 95:
						color_name = "magenta";
						break;
					case 96:
						color_name = "cyan";
						break;
					case 97:
						color_name = "white";
						break;
				}
				output += "[color=" + color_name + "]";
				active_tags.append("color");
			} else if (code == 39) {
				if (active_tags.has("color")) {
					active_tags.erase("color");
					output += "[/color]";
				}
			}
			// Background colors (40-47, 100-107, 49 reset)
			else if (code >= 40 && code <= 47 || code >= 100 && code <= 107) {
				String color_name;
				bool bright = (code >= 100);
				int base = bright ? code - 60 : code - 40;
				switch (base) {
					case 0:
						color_name = "black";
						break;
					case 1:
						color_name = "red";
						break;
					case 2:
						color_name = bright ? "lime" : "green";
						break;
					case 3:
						color_name = "yellow";
						break;
					case 4:
						color_name = "blue";
						break;
					case 5:
						color_name = "magenta";
						break;
					case 6:
						color_name = "cyan";
						break;
					case 7:
						color_name = "white";
						break;
				}
				output += "[bgcolor=" + color_name + "]";
				active_tags.append("bgcolor");
			} else if (code == 49) {
				if (active_tags.has("bgcolor")) {
					active_tags.erase("bgcolor");
					output += "[/bgcolor]";
				}
			}
			// True color foreground: 38;2;r;g;b
			else if (code == 38 && i + 3 < codes.size() && codes[i + 1].to_int() == 2) {
				int r = codes[i + 2].to_int();
				int g = codes[i + 3].to_int();
				int b = codes[i + 4].to_int();
				Color c(r / 255.0, g / 255.0, b / 255.0);
				output += "[color=" + c.to_html(false) + "]";
				active_tags.append("color");
				i += 4; // skip the parameters
			}
			// True color background: 48;2;r;g;b
			else if (code == 48 && i + 3 < codes.size() && codes[i + 1].to_int() == 2) {
				int r = codes[i + 2].to_int();
				int g = codes[i + 3].to_int();
				int b = codes[i + 4].to_int();
				Color c(r / 255.0, g / 255.0, b / 255.0);
				output += "[bgcolor=" + c.to_html(false) + "]";
				active_tags.append("bgcolor");
				i += 4;
			}
			// 256-color modes (38;5;n and 48;5;n) - we can map known ones or fall back to hex
			else if (code == 38 && i + 2 < codes.size() && codes[i + 1].to_int() == 5) {
				int idx = codes[i + 2].to_int();
				// You had specific mappings like 218 → pink, 98 → purple, 208 → orange
				String name;
				if (idx == 218) {
					name = "pink";
				} else if (idx == 98) {
					name = "purple";
				} else if (idx == 208) {
					name = "orange";
				}
				if (!name.is_empty()) {
					output += "[color=" + name + "]";
				} else {
					// Fallback: approximate RGB for common indices or just use hex if you have a palette
					// For simplicity here, we skip unknown 256 colors or you can extend
				}
				active_tags.append("color");
				i += 2;
			} else if (code == 48 && i + 2 < codes.size() && codes[i + 1].to_int() == 5) {
				// Similar handling for background 256 colors
				i += 2;
			}
		}

		pos = m_pos + 1;
	}

	// Final reset if any styles still active
	if (!active_tags.is_empty()) {
		for (int j = active_tags.size() - 1; j >= 0; --j) {
			output += "[/" + active_tags[j] + "]";
		}
	}

	return output;
}

void ConsoleLogger::_bind_methods() {
	ADD_SIGNAL(MethodInfo("log_bbcode", PropertyInfo(Variant::STRING, "bbcode")));
	ADD_SIGNAL(MethodInfo("log_raw", PropertyInfo(Variant::STRING, "raw")));
}

void ConsoleLogger::_log_error(const String &p_function, const String &p_file, int32_t p_line, const String &p_code, const String &p_rationale, bool p_editor_notify, int32_t p_error_type, const TypedArray<Ref<ScriptBacktrace>> &p_script_backtraces) {
}

void ConsoleLogger::_log_message(const String &p_message, bool p_error) {
	static StringName log_bbcode_strn = StringName("log_bbcode");
	static StringName log_raw_strn = StringName("log_bbcode");

	if (p_error) {
		emit_signal(log_bbcode_strn, vformat("[color=red][b]ERROR:[/b] %s[/color]", p_message));
	} else {
		emit_signal(log_bbcode_strn, bbcode_from_ansi(p_message));
	}
}
