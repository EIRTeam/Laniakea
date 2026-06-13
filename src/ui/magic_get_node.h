#pragma once

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/packed_scene.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/core/error_macros.hpp"

using namespace godot;

template <typename DesiredNodeType>
DesiredNodeType *get_node_type_checked(Node *p_us, NodePath p_node_path) {
	DEV_ASSERT(p_us->has_node(p_node_path));
	DesiredNodeType *out = p_us->get_node<DesiredNodeType>(p_node_path);
	DEV_ASSERT(out != nullptr);
	return out;
}

template <typename DesiredNodeType>
DesiredNodeType *instantiate_scene_type_checked(const String &p_path) {
	Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(p_path);
	DEV_ASSERT(scene.is_valid());
	Node *scene_node = scene->instantiate();
	DesiredNodeType *casted_node = Object::cast_to<DesiredNodeType>(scene_node);
	DEV_ASSERT(casted_node != nullptr);
	return casted_node;
}
