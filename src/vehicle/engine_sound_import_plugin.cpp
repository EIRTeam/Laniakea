#include "engine_sound_import_plugin.h"

#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/resource_saver.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "vehicle/engine_sound_config.h"

TypedArray<Dictionary> AngeTheGreatSimImporter::_get_import_options(const String &p_path, int32_t p_preset_index) const {
	return {};
}

String AngeTheGreatSimImporter::_get_save_extension() const {
	return "";
}

int AngeTheGreatSimImporter::_get_import_order() const {
	return 99;
}

String AngeTheGreatSimImporter::_get_resource_type() const {
	return "LNEngineSoundConfiguration";
}

String AngeTheGreatSimImporter::_get_importer_name() const {
	return "eirteam.atg_es_csv";
}

String AngeTheGreatSimImporter::_get_visible_name() const {
	return "AngeTheGreat engine simulator export";
}

PackedStringArray AngeTheGreatSimImporter::_get_recognized_extensions() const {
	return { "csv" };
}

Error AngeTheGreatSimImporter::_import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const {
	Ref<FileAccess> fa = FileAccess::open(p_source_file, FileAccess::READ);
	// skip header line
	fa->get_line();

	Ref<LNEngineSoundConfiguration> sound_config;
	sound_config.instantiate();

	while (!fa->eof_reached()) {
		PackedStringArray csv_line = fa->get_csv_line();

		if (csv_line.size() == 1 && csv_line[0].is_empty()) {
			continue;
		}

		ERR_CONTINUE_MSG(csv_line.size() != 6, vformat("Engine definition line invalid, got %d expected 6", csv_line.size()));

		const String audio_path = csv_line[0];
		const String type = csv_line[1];
		const String rpm = csv_line[2];
		const String throttle_percent = csv_line[3];
		const String loop = csv_line[4];
		const String duration_seconds = csv_line[5];

		if (type != "rpm_loop") {
			continue;
		}

		const String load_path = p_source_file.get_base_dir().path_join(csv_line[0]);

		Ref<AudioStream> audio = ResourceLoader::get_singleton()->load(load_path);

		ERR_CONTINUE_MSG(!audio.is_valid(), vformat("Couldn't load engine audio from %s", load_path));

		sound_config->add_sound(throttle_percent.to_int(), rpm.to_int(), audio);
	}

	Error saved = ResourceSaver::get_singleton()->save(sound_config, p_source_file.get_basename() + String(".res"));

	return saved;
}
