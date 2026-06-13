#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "chunkinator/chunk_spawner.h"
#include "chunkinator/chunkinator_debug_snapshot_viewer.h"
#include "chunkinator/chunkinator_debugger.h"
#include "chunkinator/chunkinator_test.h"
#include "console/console_system.h"
#include "console/cvar.h"
#include "console/gui/console_gui.h"
#include "console/gui/console_logger.h"
#include "debug/debug_overlay.h"
#include "example_class.h"
#include "game/base_character.h"
#include "game/biped_animation_base.h"
#include "game/bullet_trail.h"
#include "game/character_animation_base.h"
#include "game/character_animation_settings.h"
#include "game/character_hitbox.h"
#include "game/character_hitbox_detector.h"
#include "game/character_model.h"
#include "game/game_rules.h"
#include "game/game_rules_laniakea.h"
#include "game/main_loop.h"
#include "game/movement_settings.h"
#include "game/physics_prop.h"
#include "game/player_character.h"
#include "game/protagonist_player_character.h"
#include "game/rexbot/rexbot_configuration.h"
#include "game/rexbot/rexbot_npc_base.h"
#include "game/turret/npc_turret.h"
#include "game/ui/radial_container.h"
#include "game/ui/item_selector_ui.h"
#include "game/ui/item_select_icon.h"
#include "game/weapon_counter_shield.h"
#include "game/weapon_gravitygun.h"
#include "game/weapon_instance.h"
#include "game/weapon_firearm.h"
#include "game/weapon_model.h"
#include "game/weapon_rifle_test.h"
#include "godot_cpp/classes/editor_plugin_registration.hpp"
#include "godot_cpp/classes/resource_format_loader.hpp"
#include "godot_cpp/classes/resource_importer.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "indirect_mesh.h"
#include "animation/inertialization_skeleton_modifier_polynomial.h"
#include "quadtree.h"
#include "segment_quadtree.h"
#include "terrain_generator/terrain_heightmap_combine_layer.h"
#include "terrain_generator/terrain_manager.h"
#include "terrain_generator/terrain_settings.h"
#include "indirect_mesh_instance_3d.h"
#include "game/player_camera.h"
#include "animation/hip_rotator_modifier.h"
#include "vehicle/clutch.h"
#include "vehicle/engine_sound_config.h"
#include "vehicle/engine_sound_import_plugin.h"
#include "vehicle/shaft.h"
#include "vehicle/suspension_test.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_differential.h"
#include "vehicle/vehicle_drivetrain_config.h"
#include "vehicle/vehicle_drivetrain_debugger.h"
#include "vehicle/vehicle_engine.h"
#include "vehicle/vehicle_engine_settings.h"
#include "vehicle/vehicle_settings.h"
#include "vehicle/vehicle_suspension_macpherson_settings.h"
#include "vehicle/vehicle_suspension_settings.h"
#include "vehicle/vehicle_wheel.h"
#include "vehicle/vehicle_wheel_settings.h"
#include "vehicle/engine_sound_editor_plugin.h"
#include "vehicle/vehicle_wheel_shaft.h"

using namespace godot;

void _editor_init() {
	DebugOverlay *overlay = memnew(DebugOverlay);
	MainLoop *main_loop = Engine::get_singleton()->get_main_loop();
	overlay->initialize(Object::cast_to<SceneTree>(main_loop));
}

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		if (!Engine::get_singleton()->is_editor_hint()) {
			return;
		}
		callable_mp_static(&_editor_init).call_deferred();

		GDREGISTER_CLASS(AngeTheGreatSimImporter);
		GDREGISTER_CLASS(LNVehicleSoundEditorPlugin);
		EditorPlugins::add_by_type<LNVehicleSoundEditorPlugin>();
		return;
	}
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(CharacterAnimationSettings);
	GDREGISTER_CLASS(IndirectMesh);
	GDREGISTER_CLASS(IndirectMeshInstance3D);
	GDREGISTER_CLASS(TerrainScattererLODMesh);
	GDREGISTER_CLASS(TerrainScattererElementSettings);
	GDREGISTER_CLASS(TerrainScatterLayerSettings);
	GDREGISTER_CLASS(ExampleClass);
	GDREGISTER_CLASS(QuadTree);
	GDREGISTER_CLASS(ChunkinatorTest);
	GDREGISTER_CLASS(TerrainSettings);
	GDREGISTER_CLASS(TerrainHeightNoiseLayerSettings);
	GDREGISTER_ABSTRACT_CLASS(CVarProxy);
	GDREGISTER_ABSTRACT_CLASS(ConsoleGUI);
	GDREGISTER_CLASS(PlayerCamera);
	GDREGISTER_CLASS(LaniakeaMainLoop);
	GDREGISTER_ABSTRACT_CLASS(GameRules);
	GDREGISTER_ABSTRACT_CLASS(LaniakeaGameRules);
	GDREGISTER_CLASS(MovementSettings);
	GDREGISTER_ABSTRACT_CLASS(Chunkinator);
	GDREGISTER_ABSTRACT_CLASS(ChunkinatorDebugLayerViewer);
	GDREGISTER_ABSTRACT_CLASS(ChunkinatorDebugger);
	GDREGISTER_ABSTRACT_CLASS(ChunkSpawner);
	GDREGISTER_ABSTRACT_CLASS(TerrainManager);
	GDREGISTER_ABSTRACT_CLASS(SegmentQuadTreeDebug);
	GDREGISTER_ABSTRACT_CLASS(ConsoleLogger);
	GDREGISTER_ABSTRACT_CLASS(ConsoleSystem);
	GDREGISTER_ABSTRACT_CLASS(BaseCharacter);
	GDREGISTER_ABSTRACT_CLASS(WeaponInstanceBase);
	GDREGISTER_ABSTRACT_CLASS(WeaponFirearmInstance);
	GDREGISTER_CLASS(WeaponRifleTest);
	GDREGISTER_ABSTRACT_CLASS(BipedAnimationBase);
	GDREGISTER_CLASS(PlayerUI);
	GDREGISTER_CLASS(CharacterModel);
	GDREGISTER_CLASS(PlayerCharacter);
	GDREGISTER_CLASS(PlayerCharacterProtagonist);
	GDREGISTER_ABSTRACT_CLASS(BulletTrail);
	GDREGISTER_CLASS(HipRotatorModifier3D);
	GDREGISTER_CLASS(WeaponModel);
	GDREGISTER_CLASS(WeaponGravityGun);
	GDREGISTER_CLASS(WeaponCounterShield);
	GDREGISTER_CLASS(LaniakeaPhysicsProp);
	GDREGISTER_CLASS(RadialContainer);
	GDREGISTER_CLASS(ItemSelectorUI);
	GDREGISTER_CLASS(ItemSelectIcon);

	GDREGISTER_CLASS(CharacterHitbox);
	GDREGISTER_CLASS(CharacterHitboxDetector);
	
	GDREGISTER_CLASS(InertializationSkeletonModifierPolynomial);
	GDREGISTER_CLASS(RexbotConfiguration);
	GDREGISTER_ABSTRACT_CLASS(RexbotNPCBase);
	GDREGISTER_ABSTRACT_CLASS(DebugTextLineDrawer);
	GDREGISTER_ABSTRACT_CLASS(AnimationSequenceFuture);
	GDREGISTER_ABSTRACT_CLASS(BipedAnimationSequenceFuture);
	GDREGISTER_ABSTRACT_CLASS(DebugOverlay);
	GDREGISTER_CLASS(NPCTurret);
	GDREGISTER_ABSTRACT_CLASS(LNVehicleShaft);
	GDREGISTER_CLASS(LNVehicleEngineSettings);
	GDREGISTER_CLASS(LNVehicleDrivetrainSettings);
	GDREGISTER_CLASS(LNVehicleSettings);
	GDREGISTER_CLASS(LNVehicleWheelSettings);
	GDREGISTER_ABSTRACT_CLASS(LNVehicleSuspensionSettings);
	GDREGISTER_CLASS(LNVehicleMacPhersonSuspensionSettings);
	GDREGISTER_CLASS(LNVehicle);
	GDREGISTER_CLASS(LNVehicleWheel);
	GDREGISTER_CLASS(LNVehicleEngine);
	GDREGISTER_ABSTRACT_CLASS(LNVehicleDifferential);
	GDREGISTER_ABSTRACT_CLASS(LNVehicleGearbox);
	GDREGISTER_ABSTRACT_CLASS(LNVehicleClutchNode);
	GDREGISTER_ABSTRACT_CLASS(LNVehicleDrivetrainDebugger);
	GDREGISTER_ABSTRACT_CLASS(LNVehicleWheelShaft);
	GDREGISTER_CLASS(LNEngineSoundConfiguration);
	GDREGISTER_CLASS(SuspensionTest);
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		if (!Engine::get_singleton()->is_editor_hint()) {
			return;
		}
		memdelete(DebugOverlay::get_singleton());
		return;
	}
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT chunkinator_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}