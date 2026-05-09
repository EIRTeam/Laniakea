#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

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
#include "game/character_animation_settings.h"
#include "game/character_hitbox.h"
#include "game/character_hitbox_detector.h"
#include "game/character_model.h"
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

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
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
	GDREGISTER_CLASS(NPCTurret);
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
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