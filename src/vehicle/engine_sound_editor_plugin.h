#include "godot_cpp/classes/editor_plugin.hpp"
#include "vehicle/engine_sound_import_plugin.h"

using namespace godot;

class LNVehicleSoundEditorPlugin : public EditorPlugin {
    GDCLASS(LNVehicleSoundEditorPlugin, EditorPlugin);

    Ref<AngeTheGreatSimImporter> atg_importer;
public:
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;
    static void _bind_methods() {}
};