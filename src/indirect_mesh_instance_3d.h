#pragma once
#include "godot_cpp/classes/multi_mesh.hpp"
#include "godot_cpp/classes/rd_shader_file.hpp"
#include "godot_cpp/classes/render_data.hpp"
#include "godot_cpp/classes/visual_instance3d.hpp"
#include "indirect_mesh.h"
#include "shaders/indirect_mesh_structures.h"
#include "shaders/multimesh_lod.glsl.gen.h"

using namespace godot;
class IndirectMeshInstance3D : public VisualInstance3D {
	GDCLASS(IndirectMeshInstance3D, VisualInstance3D);
	Ref<IndirectMesh> mesh;

	IndirectMeshConfig mesh_gpu_config = {
		.radius = 0.25
	};

	RID multimesh;
	RID multimesh_config_buffer;
	RID lod_config_buffer;
	RID gpu_instace_data_buffer;
	RID gpu_cull_data_buffer;

	RID compositor;
	RID compositor_effect;
	RID compositor_effect_2;

	MultimeshLodShaderRD lod_shader;

	struct {
		RID pipeline;
		RID shader;
	} cull_shader;

	enum CullShaderUniforms {
		CULL_SHADER_MESH_CONFIG_UNIFORM = 0,
		CULL_SHADER_MESH_LOD_CONFIGS_UNIFORM = 1,
		CULL_SHADER_INPUT_TRANSFORMS = 2,
		CULL_SHADER_OUTPUT_TRANSFORMS = 3,
		CULL_SHADER_CULL_DATA_UNIFORM = 4,
		CULL_SHADER_INDIRECT_COMMAND_BUFFER_UNIFORM = 5
	};

public:
	int _get_gpu_buffer_element_size() const;
	int _get_gpu_buffer_element_stride() const;

	void _render(bool p_lod_pass);
	void _pre_opaque(int p_callback_type, RenderData *p_render_data);
	void _post_sky(int p_callback_type, RenderData *p_render_data);
	void _build_multimesh();
	void _recreate_lod_config_buffer();
	void _update_mesh_gpu_config();
	void _allocate_gpu_instance_data_buffer();
	static void _bind_methods();
	void _notification(int p_what);
	IndirectMeshInstance3D();
	~IndirectMeshInstance3D();

	Ref<IndirectMesh> get_mesh() const { return mesh; }
	void set_mesh(const Ref<IndirectMesh> &p_mesh);

	void initialize_bind(Ref<IndirectMesh> p_mesh, int p_instance_count, TypedArray<Transform3D> p_transforms);
	void initialize(Ref<IndirectMesh> p_mesh, int p_instance_count, Vector<Transform3D> p_transforms);
};
