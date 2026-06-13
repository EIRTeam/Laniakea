#include "debug_overlay.h"

#include "debug/debug_shaders.h"
#include "debug/debug_text_line_drawer.h"
#include "debug_constexpr.h"
#include "game/main_loop.h"
#include "gdextension_interface.h"
#include "godot_cpp/classes/cylinder_mesh.hpp"
#include "godot_cpp/classes/cylinder_shape3d.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/geometry_instance3d.hpp"
#include "godot_cpp/classes/immediate_mesh.hpp"
#include "godot_cpp/classes/main_loop.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/resource_saver.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/sphere_shape3d.hpp"
#include "godot_cpp/classes/text_mesh.hpp"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"

DebugOverlay *DebugOverlay::singleton = nullptr;
CVar DebugOverlay::overlays_frozen_cvar = CVar::create_variable("debug_overlays.frozen", GDEXTENSION_VARIANT_TYPE_BOOL, false, "If 1 freezes debug overlays", PROPERTY_HINT_NONE, "");

void DebugOverlay::initialize(SceneTree *p_main_loop) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	singleton = this;
	root_node = memnew(Node3D);
	p_main_loop->get_root()->add_child(root_node);
	root_node->set_as_top_level(true);

	text_line_drawer_physics = memnew(DebugTextLineDrawer);
	text_line_drawer_process = memnew(DebugTextLineDrawer);
	root_node->add_child(text_line_drawer_physics);
	root_node->add_child(text_line_drawer_process);

	// Cylinder mesh
	{
		Ref<CylinderShape3D> cylinder_shape;
		cylinder_shape.instantiate();
		cylinder_shape->set_radius(1.0f);
		cylinder_shape->set_height(1.0f);
		cylinder_mesh = cylinder_shape->get_debug_mesh();
	}

	{
		Ref<CylinderMesh> cone;
		cone.instantiate();
		cone->set_height(1.0f);
		cone->set_top_radius(0.0f);
		cone->set_bottom_radius(1.0f);
		cone_mesh = cone;
	}

	// Sphere mesh
	{
		Ref<SphereShape3D> sphere_shape;
		sphere_shape.instantiate();
		sphere_shape->set_radius(1.0f);
		sphere_mesh = sphere_shape->get_debug_mesh();
	}

	// Debug material
	{
		debug_overlay_material.instantiate();
		Ref<Shader> shader;
		shader.instantiate();
		shader->set_code(debug_shader);
		debug_overlay_material->set_shader(shader);
	}

	// Debug material (no depth)
	{
		debug_overlay_material_nodepth.instantiate();
		Ref<Shader> shader;
		shader.instantiate();
		shader->set_code(debug_shader_nodepth);
		debug_overlay_material_nodepth->set_shader(shader);
	}

	// Debug material for point rendering
	{
		debug_overlay_material_point.instantiate();
		Ref<Shader> shader;
		shader.instantiate();
		shader->set_code(debug_shader_point);
		debug_overlay_material_point->set_shader(shader);
	}

	// Circle mesh
	{
		constexpr int CIRCLE_MESH_RESOLUTION = 64;
		circle_mesh.instantiate();
		circle_mesh_solid.instantiate();

		Ref<ImmediateMesh> circle_mesh_im;
		Ref<ImmediateMesh> circle_mesh_im_solid;
		circle_mesh_im.instantiate();
		circle_mesh_im_solid.instantiate();

		circle_mesh_im_solid->surface_begin(Mesh::PRIMITIVE_TRIANGLES, debug_overlay_material);
		circle_mesh_im->surface_begin(Mesh::PRIMITIVE_LINE_STRIP, debug_overlay_material);

		const Vector3 vector_right = Vector3(1.0f, 0.0f, 0.0f);
		const Vector3 vector_up = Vector3(0.0f, 1.0f, 0.0f);

		for (int i = 0; i < CIRCLE_MESH_RESOLUTION; i++) {
			const float progress = i / static_cast<float>(CIRCLE_MESH_RESOLUTION);
			const float progress_next = (i + 1) / static_cast<float>(CIRCLE_MESH_RESOLUTION);
			const Vector3 pos = vector_right.rotated(vector_up, progress * Math_TAU);
			const Vector3 pos_next = vector_right.rotated(vector_up, progress_next * Math_TAU);

			circle_mesh_im_solid->surface_add_vertex(Vector3());
			circle_mesh_im_solid->surface_add_vertex(pos_next);
			circle_mesh_im_solid->surface_add_vertex(pos);

			if (i == 0) {
				circle_mesh_im->surface_add_vertex(pos);
			}
			circle_mesh_im->surface_add_vertex(pos_next);
		}

		circle_mesh_im->surface_end();
		circle_mesh_im_solid->surface_end();

		circle_mesh = circle_mesh_im;
		circle_mesh_solid = circle_mesh_im_solid;
	}

	// HACK to allow this to work in the editor
	if (Engine::get_singleton()->is_editor_hint()) {
		p_main_loop->connect("physics_frame", callable_mp(this, &DebugOverlay::advance).bind(ProcessPass::PHYSICS));
	}
}

MeshInstance3D *DebugOverlay::_create_mesh_instance(const Ref<Mesh> &p_mesh, const Color &p_color, const bool p_depth_test) {
	if constexpr (!Debug::is_debug_enabled) {
		return nullptr;
	}

	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(p_mesh);
	mi->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	mi->set_material_override(p_depth_test ? singleton->debug_overlay_material : singleton->debug_overlay_material_nodepth);
	static StringName color_shader_paramter = "color";
	mi->set_instance_shader_parameter(color_shader_paramter, p_color);
	if (!p_depth_test) {
		mi->set_sorting_offset(1000000.0f);
	}
	return mi;
}

void DebugOverlay::_dispose_overlay(int p_idx) {
	ERR_FAIL_INDEX(p_idx, overlays.size());
	for (Node3D *node : overlays[p_idx].nodes) {
		node->queue_free();
	}
	overlays.remove_at_unordered(p_idx);
}

void DebugOverlay::advance(ProcessPass p_pass) {
	if (!Engine::get_singleton()->is_editor_hint() && overlays_frozen_cvar.get_bool()) {
		return;
	}

	DebugTextLineDrawer *text_line_drawer = p_pass == ProcessPass::PHYSICS ? text_line_drawer_physics : text_line_drawer_process;

	text_line_drawer->clear_strings();

	for (int i = overlays.size() - 1; i >= 0; i--) {
		const Overlay &overlay = overlays[i];
		if (overlay.process_pass != p_pass) {
			continue;
		}

		if (overlay.debug_text.has_value()) {
			text_line_drawer->add_string(overlay.debug_text->world_pos, overlay.debug_text->text);
		}

		if (overlay.end_time <= (Time::get_singleton()->get_ticks_usec() / 1000000.0f)) {
			_dispose_overlay(i);
		}
	}
}

void DebugOverlay::_register_overlay(const DebugOverlay::Overlay &p_overlay) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	if (!Engine::get_singleton()->is_editor_hint() && overlays_frozen_cvar.get_bool()) {
		for (Node3D *node : p_overlay.nodes) {
			node->queue_free();
		}
		return;
	}

	const bool in_physics = Engine::get_singleton()->is_in_physics_frame();

	for (Node3D *node : p_overlay.nodes) {
		root_node->add_child(node);
	}
	DebugOverlay::Overlay overlay = p_overlay;
	overlay.process_pass = in_physics ? PHYSICS : PROCESS;
	overlays.push_back(overlay);
}

void DebugOverlay::sphere(const Vector3 &p_center, const float p_radius, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	MeshInstance3D *mi = _create_mesh_instance(singleton->sphere_mesh, p_color, p_depth_test);
	mi->set_transform(Transform3D(Basis::from_scale(Vector3(p_radius, p_radius, p_radius)), p_center));

	Vector<Node3D *> nodes;
	nodes.push_back(mi);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::line(const Vector3 &p_from, const Vector3 &p_to, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}
	Ref<ImmediateMesh> im;
	im.instantiate();
	im->surface_begin(Mesh::PRIMITIVE_LINES);
	im->surface_add_vertex(p_from);
	im->surface_add_vertex(p_to);
	im->surface_end();

	MeshInstance3D *mi = _create_mesh_instance(im, p_color, p_depth_test);
	Vector<Node3D *> nodes;
	nodes.push_back(mi);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::cylinder(const Vector3 &p_at, const float p_height, const float p_radius, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	MeshInstance3D *mi = _create_mesh_instance(singleton->cylinder_mesh, p_color, p_depth_test);
	mi->set_transform(Transform3D(Basis::from_scale(Vector3(p_radius, p_height, p_radius)), p_at));
	Vector<Node3D *> nodes;
	nodes.push_back(mi);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::cylinder_with_direction(const Vector3 &p_at, const Vector3 &p_dir, const float p_height, const float p_radius, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	MeshInstance3D *mi = _create_mesh_instance(singleton->cylinder_mesh, p_color, p_depth_test);

	mi->set_transform(Transform3D(Basis::looking_at(p_dir).scaled_local(Vector3(p_radius, p_radius, p_radius)), p_at).rotated_local(Vector3(1.0, 0.0, 0.0), -Math_PI * 0.5));
	Vector<Node3D *> nodes;
	nodes.push_back(mi);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::horz_arrow(const Vector3 &p_from, const Vector3 &p_to, const float p_width, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}
	// Build arrow mesh
	Vector3 dir = p_from.direction_to(p_to);
	const Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 side = dir.cross(up).normalized();
	Ref<ImmediateMesh> im;
	im.instantiate();
	im->surface_begin(Mesh::PRIMITIVE_LINE_STRIP);

	const float shaft_size = p_width * 0.5f;
	im->surface_add_vertex(p_from + side * shaft_size);
	im->surface_add_vertex(p_to - dir * p_width + side * shaft_size);
	im->surface_add_vertex(p_to - dir * p_width + side * p_width);
	im->surface_add_vertex(p_to);
	im->surface_add_vertex(p_to - dir * p_width - side * p_width);
	im->surface_add_vertex(p_to - dir * p_width - side * shaft_size);
	im->surface_add_vertex(p_from - side * shaft_size);
	im->surface_end();

	MeshInstance3D *mi = _create_mesh_instance(im, p_color, p_depth_test);
	Vector<Node3D *> nodes;
	nodes.push_back(mi);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::vert_arrow(const Vector3 &p_from, const Vector3 &p_to, const float p_width, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}
	// Build arrow mesh
	Vector3 dir = p_from.direction_to(p_to);
	Vector3 side = Vector3(0.0f, -1.0f, 0.0f);
	Ref<ImmediateMesh> im;
	im.instantiate();
	im->surface_begin(Mesh::PRIMITIVE_LINE_STRIP);

	const float shaft_size = p_width * 0.5f;
	im->surface_add_vertex(p_from + side * shaft_size);
	im->surface_add_vertex(p_to - dir * p_width + side * shaft_size);
	im->surface_add_vertex(p_to - dir * p_width + side * p_width);
	im->surface_add_vertex(p_to);
	im->surface_add_vertex(p_to - dir * p_width - side * p_width);
	im->surface_add_vertex(p_to - dir * p_width - side * shaft_size);
	im->surface_add_vertex(p_from - side * shaft_size);
	im->surface_end();

	MeshInstance3D *mi = _create_mesh_instance(im, p_color, p_depth_test);
	Vector<Node3D *> nodes;
	nodes.push_back(mi);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::filled_arrow(const Vector3 &p_from, const Vector3 &p_to, const float p_width, const Color &p_color, const bool p_depth_test, const float p_duration) {
	const float distance = p_from.distance_to(p_to);
	const Vector3 forward = p_from.direction_to(p_to);

	Vector3 up = Vector3(0.0, 1.0, 0.0);
	if (forward.cross(up).is_zero_approx()) {
		up = Vector3(1.0, 0.0, 0.0);
	}

	const Basis base_matrix = Basis::looking_at(forward, up).rotated_local(Vector3(1.0, 0.0, 0.0), -Math_PI * 0.5);

	Vector<Node3D *> nodes;

	{
		MeshInstance3D *shaft_mi = _create_mesh_instance(singleton->cylinder_mesh, p_color, p_depth_test);
		constexpr float shaft_width_multiplier = 0.25f;
		const Vector3 shaft_center = p_from + (forward * (distance - p_width)) * 0.5f;
		shaft_mi->set_position(shaft_center);
		shaft_mi->set_basis(base_matrix.scaled_local(Vector3(p_width * 0.5f * shaft_width_multiplier, distance - p_width, p_width * 0.5f * shaft_width_multiplier)));
		nodes.push_back(shaft_mi);
	}

	{
		MeshInstance3D *tip_mi = _create_mesh_instance(singleton->cone_mesh, p_color, p_depth_test);
		const Vector3 tip_center = p_from + (forward * (distance - p_width * 0.5f));
		tip_mi->set_position(tip_center);
		tip_mi->set_basis(base_matrix.scaled_local(Vector3(p_width * 0.5f, p_width, p_width * 0.5f)));
		nodes.push_back(tip_mi);
	}

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::path(const PackedVector3Array p_path, bool p_draw_points, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}
	Ref<ImmediateMesh> im;
	im.instantiate();
	im->surface_begin(Mesh::PRIMITIVE_LINE_STRIP);

	for (const Vector3 &p : p_path) {
		im->surface_add_vertex(p);
	}

	im->surface_end();

	if (p_draw_points) {
		im->surface_begin(Mesh::PRIMITIVE_POINTS, singleton->debug_overlay_material_point);
		for (const Vector3 &p : p_path) {
			im->surface_add_vertex(p);
		}
		im->surface_end();
	}

	MeshInstance3D *mi = _create_mesh_instance(im, p_color, p_depth_test);
	Vector<Node3D *> nodes;
	nodes.push_back(mi);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::horz_circle(const Vector3 &p_at, const float p_radius, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	Overlay overlay = {
		.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration
	};

	if (p_color.a > 0.0f) {
		MeshInstance3D *mesh_solid = _create_mesh_instance(singleton->circle_mesh_solid, p_color, p_depth_test);
		mesh_solid->set_position(p_at);
		mesh_solid->set_scale(Vector3(p_radius, p_radius, p_radius));
		overlay.nodes.push_back(mesh_solid);
	}

	const Color color_without_alpha = Color(p_color.r, p_color.g, p_color.b);

	MeshInstance3D *mesh = _create_mesh_instance(singleton->circle_mesh, color_without_alpha, p_depth_test);

	mesh->set_position(p_at);
	mesh->set_scale(Vector3(p_radius, 1.0f, p_radius));

	overlay.nodes.push_back(mesh);

	singleton->_register_overlay(overlay);
}

void DebugOverlay::circle_with_dir(const Vector3 &p_at, const Vector3 &p_dir, const float p_radius, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	Overlay overlay = {
		.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration
	};

	Basis basis = Basis::looking_at(p_dir).rotated_local(Vector3(1.0, 0.0, 0.0), -Math_PI * 0.5).scaled_local(Vector3(p_radius, 1.0f, p_radius));

	if (p_color.a > 0.0f) {
		MeshInstance3D *mesh_solid = _create_mesh_instance(singleton->circle_mesh_solid, p_color, p_depth_test);
		mesh_solid->set_position(p_at);
		mesh_solid->set_basis(basis);
		overlay.nodes.push_back(mesh_solid);
	}

	const Color color_without_alpha = Color(p_color.r, p_color.g, p_color.b);

	MeshInstance3D *mesh = _create_mesh_instance(singleton->circle_mesh, color_without_alpha, p_depth_test);

	mesh->set_position(p_at);
	mesh->set_basis(basis);

	overlay.nodes.push_back(mesh);

	singleton->_register_overlay(overlay);
}

void DebugOverlay::cone(const Vector3 &p_from, const Vector3 &p_to, const float p_angle, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	const int CONE_MESH_RESOLUTION = 16;

	const Vector3 diff = p_to - p_from;
	const float diff_length = diff.length();
	const Vector3 dir = diff / diff_length;

	Ref<CylinderMesh> cylinder_mesh;
	cylinder_mesh.instantiate();
	cylinder_mesh->set_bottom_radius(0.0f);
	cylinder_mesh->set_height(diff_length);

	cylinder_mesh->set_top_radius(cylinder_mesh->get_height() * Math::tan(p_angle));
	MeshInstance3D *mi = _create_mesh_instance(cylinder_mesh, p_color, p_depth_test);
	mi->set_position(p_from + dir * (diff_length * 0.5f));
	Basis new_basis;
	new_basis.set_column(0, dir.cross(Vector3(0.0f, 1.0f, 0.0f)).normalized());
	new_basis.set_column(1, dir);
	new_basis.set_column(2, new_basis.get_column(0).cross(new_basis.get_column(1)).normalized());

	mi->set_basis(new_basis);

	Vector<Node3D *> nodes;
	nodes.push_back(mi);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });

	DebugOverlay::line(p_from, p_to, Color(1.0, 0.0, 0.0), false);
}

void DebugOverlay::text(const Vector3 &p_at, const String &p_text, const Color &p_color, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}

	singleton->_register_overlay({ .debug_text = DebugText {
										   .world_pos = p_at,
										   .text = p_text },
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::mesh_with_trf(const Transform3D &p_trf, const Ref<Mesh> &p_mesh, const bool p_depth_test, const float p_duration) {
	if constexpr (!Debug::is_debug_enabled) {
		return;
	}
	MeshInstance3D *mesh = _create_mesh_instance(p_mesh, Color(1.0, 1.0, 1.0, 1.0), p_depth_test);
	mesh->set_material_override(Ref<Material>());
	if (!p_mesh.is_valid()) {
		print_line("INVALI!?");
	}
	mesh->set_transform(p_trf);

	ResourceSaver::get_singleton()->save(p_mesh, "res://debugmesh.tres");

	Vector<Node3D *> nodes;
	nodes.push_back(mesh);

	singleton->_register_overlay({ .nodes = nodes,
			.end_time = (Time::get_singleton()->get_ticks_usec() / 1000000.0f) + p_duration });
}

void DebugOverlay::_bind_methods() {
	ClassDB::bind_static_method("DebugOverlay", D_METHOD("line", "from", "to", "color", "depth_test", "duration"), &DebugOverlay::line, DEFVAL(true), DEFVAL(0.0f));
	ClassDB::bind_static_method("DebugOverlay", D_METHOD("sphere", "center", "radius", "color", "depth_test", "duration"), &DebugOverlay::sphere, DEFVAL(true), DEFVAL(0.0f));
}

DebugOverlay::~DebugOverlay() {
	// No actual need to destruct root_node becuase it's destroyed by the tree already
	overlays.clear();
}

DebugOverlay *DebugOverlay::get_singleton() {
	return singleton;
}
