#pragma once

#include "godot_cpp/classes/editor_import_plugin.hpp"
#include "godot_cpp/classes/editor_plugin.hpp"

using namespace godot;

class AngeTheGreatSimImporter : public EditorImportPlugin {
	GDCLASS(AngeTheGreatSimImporter, EditorImportPlugin);

public:
	virtual TypedArray<Dictionary> _get_import_options(const String &p_path, int32_t p_preset_index) const override;
	virtual String _get_save_extension() const override;
	virtual int _get_import_order() const override;
	virtual String _get_resource_type() const override;
	virtual String _get_importer_name() const override;
	virtual String _get_visible_name() const override;
	virtual PackedStringArray _get_recognized_extensions() const override;
	virtual Error _import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const override;
	static void _bind_methods() {}
};
