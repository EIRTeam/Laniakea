#include "indirect_mesh_instance_3d.h"

#include "bind_macros.h"
#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/rd_shader_source.hpp"
#include "godot_cpp/classes/rd_shader_spirv.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/uniform_set_cache_rd.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/projection.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "shaders/indirect_mesh_structures.h"
#include "shaders/multimesh_lod.glsl.gen.h"
#include "transform_storage.h"

int IndirectMeshInstance3D::_get_gpu_buffer_element_size() const {
	return 3 * 4 * sizeof(float);
}

int IndirectMeshInstance3D::_get_gpu_buffer_element_stride() const {
	return 3 * 4;
}

void IndirectMeshInstance3D::_render(bool p_lod_pass) {
	RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
	Camera3D *camera = get_viewport()->get_camera_3d();
	Transform3D camera_trf = camera->get_camera_transform();

	const Projection camera_proj_original = camera->get_camera_projection();
	Projection camera_proj = camera->get_camera_projection();
	Projection correction;
	correction.set_depth_correction(true);
	camera_proj = correction * camera_proj;
	PackedByteArray cull_data_arr;
	cull_data_arr.resize(sizeof(CullData));
	CullData *cull_data = (CullData *)cull_data_arr.ptrw();
	Plane plane_x = camera_proj.get_projection_plane(Projection::PLANE_RIGHT).normalized();
	Plane plane_y = camera_proj.get_projection_plane(Projection::PLANE_BOTTOM).normalized();

	cull_data->frustum_planes[0] = plane_x.normal.x;
	cull_data->frustum_planes[1] = plane_x.normal.z;
	cull_data->frustum_planes[2] = plane_y.normal.y;
	cull_data->frustum_planes[3] = plane_y.normal.z;
	cull_data->shadow_pass = p_lod_pass;

	TransformStorage::store_transform(camera_trf.affine_inverse().scaled(Vector3(1.0, 1.0, -1.0)), cull_data->view);
	cull_data->znear = camera_proj_original.get_z_near();
	cull_data->zfar = camera_proj_original.get_z_far();
	cull_data->P00 = camera_proj.columns[0][0];
	cull_data->P11 = camera_proj.columns[1][1];
	cull_data->camera_position[0] = camera->get_global_position().x;
	cull_data->camera_position[1] = camera->get_global_position().y;
	cull_data->camera_position[2] = camera->get_global_position().z;
	cull_data->camera_position[3] = 1.0f;

	Ref<RDUniform> mesh_config_uniform;
	mesh_config_uniform.instantiate();
	mesh_config_uniform->set_binding(CULL_SHADER_MESH_CONFIG_UNIFORM);
	mesh_config_uniform->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	mesh_config_uniform->add_id(multimesh_config_buffer);

	Ref<RDUniform> lod_configs_uniform;
	lod_configs_uniform.instantiate();
	lod_configs_uniform->set_binding(CULL_SHADER_MESH_LOD_CONFIGS_UNIFORM);
	lod_configs_uniform->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	lod_configs_uniform->add_id(lod_config_buffer);

	Ref<RDUniform> input_transforms_uniform;
	input_transforms_uniform.instantiate();
	input_transforms_uniform->set_binding(CULL_SHADER_INPUT_TRANSFORMS);
	input_transforms_uniform->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	input_transforms_uniform->add_id(gpu_instace_data_buffer);

	Ref<RDUniform> output_transforms_uniform;
	output_transforms_uniform.instantiate();
	output_transforms_uniform->set_binding(CULL_SHADER_OUTPUT_TRANSFORMS);
	output_transforms_uniform->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	output_transforms_uniform->add_id(RenderingServer::get_singleton()->multimesh_get_buffer_rd_rid(multimesh));

	Ref<RDUniform> cull_data_uniform;
	cull_data_uniform.instantiate();
	cull_data_uniform->set_binding(CULL_SHADER_CULL_DATA_UNIFORM);
	cull_data_uniform->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	cull_data_uniform->add_id(gpu_cull_data_buffer);

	Ref<RDUniform> indirect_command_buffer_uniform;
	indirect_command_buffer_uniform.instantiate();
	indirect_command_buffer_uniform->set_binding(CULL_SHADER_INDIRECT_COMMAND_BUFFER_UNIFORM);
	indirect_command_buffer_uniform->set_uniform_type(godot::RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	indirect_command_buffer_uniform->add_id(RenderingServer::get_singleton()->multimesh_get_command_buffer_rd_rid(multimesh));

	TypedArray<RDUniform> uniforms;
	uniforms.push_back(mesh_config_uniform);
	uniforms.push_back(lod_configs_uniform);
	uniforms.push_back(input_transforms_uniform);
	uniforms.push_back(output_transforms_uniform);
	uniforms.push_back(cull_data_uniform);
	uniforms.push_back(indirect_command_buffer_uniform);

	RID uniform = UniformSetCacheRD::get_cache(cull_shader.shader, 0, uniforms);

	rd->draw_command_begin_label(p_lod_pass ? "LOD" : "Cull", Color(1.0, 0.0, 0.0));
	int64_t compute_list = rd->compute_list_begin();
	rd->compute_list_bind_compute_pipeline(compute_list, cull_shader.pipeline);
	rd->compute_list_bind_uniform_set(compute_list, uniform, 0);
	rd->compute_list_set_push_constant(compute_list, cull_data_arr, cull_data_arr.size());
	rd->compute_list_dispatch(compute_list, mesh_gpu_config.instance_count, 1, 1);
	rd->compute_list_end();
	rd->draw_command_end_label();
}

void IndirectMeshInstance3D::_pre_opaque(int p_callback_type, RenderData *p_render_data) {
	_render(false);
}

void IndirectMeshInstance3D::_post_sky(int p_callback_type, RenderData *p_render_data) {
	_render(true);
}

void IndirectMeshInstance3D::_build_multimesh() {
	_recreate_lod_config_buffer();
}

void IndirectMeshInstance3D::_recreate_lod_config_buffer() {
	if (!mesh.is_valid() || mesh->get_total_surface_count() == 0) {
		return;
	}
	RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
	if (lod_config_buffer.is_valid()) {
		rd->free_rid(lod_config_buffer);
	}

	PackedByteArray gpu_lod_configs;
	gpu_lod_configs.resize(sizeof(IndirectMeshLODConfig) * mesh->get_lod_count());
	IndirectMeshLODConfig *lod_configs = (IndirectMeshLODConfig *)gpu_lod_configs.ptrw();

	int w_surface_idx = 0;

	for (int lod_idx = 0; lod_idx < mesh->get_lod_count(); lod_idx++) {
		Ref<Mesh> lod_mesh = mesh->lod_get_mesh(lod_idx);
		IndirectMeshLODConfig lod_config {
			.surface_count = lod_mesh->get_surface_count(),
			.surfaces = { -1, -1, -1, -1 }
		};
		lod_config.begin_distance = mesh->lod_get_lod_begin_distance(lod_idx);
		for (int surf_idx = 0; surf_idx < lod_mesh->get_surface_count(); surf_idx++) {
			lod_config.surfaces[surf_idx] = w_surface_idx++;
		}
		lod_configs[lod_idx] = lod_config;
	}

	lod_config_buffer = rd->storage_buffer_create(gpu_lod_configs.size(), gpu_lod_configs);
}

void IndirectMeshInstance3D::_update_mesh_gpu_config() {
	RenderingServer *rs = RenderingServer::get_singleton();
	RenderingDevice *rd = rs->get_rendering_device();
	PackedByteArray data_arr;
	data_arr.resize(sizeof(IndirectMeshConfig));

	memcpy(data_arr.ptrw(), (void *)&mesh_gpu_config, data_arr.size());

	rd->buffer_update(multimesh_config_buffer, 0, data_arr.size(), data_arr);
}

void IndirectMeshInstance3D::_allocate_gpu_instance_data_buffer() {
	RenderingServer *rs = RenderingServer::get_singleton();
	RenderingDevice *rd = rs->get_rendering_device();
	if (gpu_instace_data_buffer.is_valid()) {
		rd->free_rid(gpu_instace_data_buffer);
		gpu_instace_data_buffer = RID();
	}

	if (mesh_gpu_config.instance_count > 0) {
		gpu_instace_data_buffer = rd->storage_buffer_create(_get_gpu_buffer_element_size() * mesh_gpu_config.instance_count);
	}
}

void IndirectMeshInstance3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("initialize", "mesh", "instance_count", "transforms"), &IndirectMeshInstance3D::initialize_bind);
}

void IndirectMeshInstance3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			RenderingServer *rs = RenderingServer::get_singleton();
			rs->scenario_set_compositor(get_viewport()->get_world_3d()->get_scenario(), compositor);
			//RenderingServer::get_singleton()->call_on_render_thread(callable_mp(this, &IndirectMeshInstance3D::_render).bind(false));
		} break;
		case NOTIFICATION_ENTER_TREE: {
			RenderingServer *rs = RenderingServer::get_singleton();
			compositor = rs->compositor_create();
			compositor_effect = rs->compositor_effect_create();
			compositor_effect_2 = rs->compositor_effect_create();
			TypedArray<RID> effects;
			effects.push_back(compositor_effect);
			effects.push_back(compositor_effect_2);
			rs->compositor_set_compositor_effects(compositor, effects);
			rs->compositor_effect_set_callback(compositor_effect, godot::RenderingServer::COMPOSITOR_EFFECT_CALLBACK_TYPE_POST_TRANSPARENT, callable_mp(this, &IndirectMeshInstance3D::_post_sky));
			rs->compositor_effect_set_callback(compositor_effect_2, godot::RenderingServer::COMPOSITOR_EFFECT_CALLBACK_TYPE_PRE_OPAQUE, callable_mp(this, &IndirectMeshInstance3D::_pre_opaque));
		} break;
	}
}

IndirectMeshInstance3D::IndirectMeshInstance3D() {
	RenderingServer *rs = RenderingServer::get_singleton();
	multimesh = rs->multimesh_create();
	RenderingDevice *rd = rs->get_rendering_device();
	multimesh_config_buffer = rd->storage_buffer_create(sizeof(IndirectMeshConfig));
}

IndirectMeshInstance3D::~IndirectMeshInstance3D() {
	if (multimesh.is_valid()) {
		RenderingServer::get_singleton()->free_rid(multimesh);
	}
	RenderingServer *rs = RenderingServer::get_singleton();
	RenderingDevice *rd = rs->get_rendering_device();
	if (multimesh_config_buffer.is_valid()) {
		rd->free_rid(multimesh_config_buffer);
	}

	if (gpu_cull_data_buffer.is_valid()) {
		rd->free_rid(gpu_cull_data_buffer);
	}

	if (lod_config_buffer.is_valid()) {
		rd->free_rid(lod_config_buffer);
	}

	if (gpu_instace_data_buffer.is_valid()) {
		rd->free_rid(gpu_instace_data_buffer);
	}

	if (multimesh.is_valid()) {
		rs->free_rid(multimesh);
	}

	if (cull_shader.pipeline.is_valid()) {
		rd->free_rid(cull_shader.pipeline);
	}

	if (compositor_effect_2.is_valid()) {
		rs->free_rid(compositor_effect_2);
	}
	if (compositor_effect.is_valid()) {
		rs->free_rid(compositor_effect);
	}

	if (compositor.is_valid()) {
		rs->free_rid(compositor);
	}
}

void IndirectMeshInstance3D::initialize_bind(Ref<IndirectMesh> p_mesh, int p_instance_count, TypedArray<Transform3D> p_transforms) {
	Vector<Transform3D> transforms;
	transforms.resize(p_transforms.size());
	Transform3D *transforms_w = transforms.ptrw();
	for (int i = 0; i < p_transforms.size(); i++) {
		transforms_w[i] = p_transforms[i];
	}
	initialize(p_mesh, p_instance_count, transforms);
}

void IndirectMeshInstance3D::initialize(Ref<IndirectMesh> p_mesh, int p_instance_count, Vector<Transform3D> p_transforms) {
	mesh = p_mesh;
	mesh_gpu_config.instance_count = p_instance_count;
	mesh_gpu_config.lod_count = p_mesh->get_lod_count();
	mesh_gpu_config.transform_stride = _get_gpu_buffer_element_stride();

	_recreate_lod_config_buffer();
	_allocate_gpu_instance_data_buffer();

	PackedByteArray initial_data;
	initial_data.resize(p_instance_count * _get_gpu_buffer_element_size());

	float *w = (float *)initial_data.ptrw();

	const Transform3D identity = Transform3D();

	if (p_transforms.size() == mesh_gpu_config.instance_count) {
		for (int i = 0; i < mesh_gpu_config.instance_count; i++) {
			const int stride = _get_gpu_buffer_element_stride();
			w[i * stride + 0] = p_transforms[i].basis.rows[0][0];
			w[i * stride + 1] = p_transforms[i].basis.rows[0][1];
			w[i * stride + 2] = p_transforms[i].basis.rows[0][2];
			w[i * stride + 3] = p_transforms[i].origin.x;
			w[i * stride + 4] = p_transforms[i].basis.rows[1][0];
			w[i * stride + 5] = p_transforms[i].basis.rows[1][1];
			w[i * stride + 6] = p_transforms[i].basis.rows[1][2];
			w[i * stride + 7] = p_transforms[i].origin.y;
			w[i * stride + 8] = p_transforms[i].basis.rows[2][0];
			w[i * stride + 9] = p_transforms[i].basis.rows[2][1];
			w[i * stride + 10] = p_transforms[i].basis.rows[2][2];
			w[i * stride + 11] = p_transforms[i].origin.z;
		}
	} else {
		const Transform3D identity = Transform3D();
		for (int i = 0; i < mesh_gpu_config.instance_count * _get_gpu_buffer_element_stride(); i += _get_gpu_buffer_element_stride()) {
			w[i + 0] = identity.basis.rows[0][0];
			w[i + 1] = identity.basis.rows[0][1];
			w[i + 2] = identity.basis.rows[0][2];
			w[i + 3] = identity.origin.x;
			w[i + 4] = identity.basis.rows[1][0];
			w[i + 5] = identity.basis.rows[1][1];
			w[i + 6] = identity.basis.rows[1][2];
			w[i + 7] = identity.origin.y;
			w[i + 8] = identity.basis.rows[2][0];
			w[i + 9] = identity.basis.rows[2][1];
			w[i + 10] = identity.basis.rows[2][2];
			w[i + 11] = identity.origin.z;
		}
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	RenderingDevice *rd = rs->get_rendering_device();
	rd->buffer_update(gpu_instace_data_buffer, 0, initial_data.size(), initial_data);

	RenderingServer::get_singleton()->multimesh_allocate_data(multimesh, mesh_gpu_config.instance_count, RenderingServer::MULTIMESH_TRANSFORM_3D, false, false, true);
	RenderingServer::get_singleton()->multimesh_set_mesh(multimesh, mesh->build_combined_mesh()->get_rid());
	RenderingServer::get_singleton()->multimesh_set_custom_aabb(multimesh, AABB(Vector3(-100, -100, -100), Vector3(200, 200, 200)));

	mesh_gpu_config.radius = mesh->get_mesh_radius() * 0.1;
	_update_mesh_gpu_config();

	cull_shader.shader = lod_shader.get_shader_rid();
	cull_shader.pipeline = rd->compute_pipeline_create(cull_shader.shader);

	gpu_cull_data_buffer = rd->storage_buffer_create(sizeof(CullData));

	set_base(multimesh);

	set_process(true);
}
