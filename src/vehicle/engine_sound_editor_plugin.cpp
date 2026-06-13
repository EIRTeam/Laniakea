#include "engine_sound_editor_plugin.h"

void LNVehicleSoundEditorPlugin::_enter_tree() {
	atg_importer.instantiate();
	add_import_plugin(atg_importer);
}

void LNVehicleSoundEditorPlugin::_exit_tree() {
	remove_import_plugin(atg_importer);
	atg_importer.unref();
}
