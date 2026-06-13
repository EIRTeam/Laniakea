#include "physics_prop.h"

#include "physics_layers.h"
LaniakeaPhysicsProp::LaniakeaPhysicsProp() {
	set_collision_layer(PhysicsLayers::LAYER_PROPS);
	set_collision_mask(PhysicsLayers::LAYER_PROPS | PhysicsLayers::LAYER_WORLDSPAWN);
}
