#[compute]

#version 450

#include "indirect_mesh_structures_inc.glsl"

layout(local_size_x = 8) in;

layout(set = 0, binding = 0, std430) restrict buffer MeshConfig {
    IndirectMeshConfig config;
} mesh_config;

layout(set = 0, binding = 1, std430) restrict buffer IndirectMeshLODConfigs {
    IndirectMeshLODConfig configs[];
}
lod_configs;

layout(set = 0, binding = 2, std430) restrict buffer IndirectMeshTransforms {
    float transforms[];
}
input_transforms;

layout(set = 0, binding = 3, std430) restrict buffer OutputMeshTransforms {
    float transforms[];
}
output_transforms;

layout(push_constant, std430) uniform Params {
    CullData data;
}
cull_data;

layout(set = 0, binding = 5, std430) restrict buffer IndirectCommandBuffer {
    IndirectMeshCommandBufferCommand commands[];
}
indirect;

shared int last_transform_batch_idx;

bool IsVisible(uint p_object_index)
{
	//grab sphere cull data from the object buffer
    int stride = mesh_config.config.transform_stride;

    float radius = mesh_config.config.radius;
    vec3 center = vec3(input_transforms.transforms[p_object_index * stride + 3], input_transforms.transforms[p_object_index * stride + 7], input_transforms.transforms[p_object_index * stride + 11]);

	center = (cull_data.data.view * vec4(center,1.f)).xyz;

	bool visible = true;

	//frustrum culling
	visible = visible && center.z * cull_data.data.frustum_planes[1] - abs(center.x) * cull_data.data.frustum_planes[0] > -radius;
	visible = visible && center.z * cull_data.data.frustum_planes[3] - abs(center.y) * cull_data.data.frustum_planes[2] > -radius;

	// the near/far plane culling uses camera space Z directly
    visible = visible && center.z + radius > cull_data.data.znear && center.z - radius < cull_data.data.zfar;

	//visible = visible;// || cull_data.data.cullingEnabled == 0;
	return visible;
}

int run_lod_selection() {
    if (gl_GlobalInvocationID.x >= mesh_config.config.instance_count) {
        return -1;
    }

    int trf_stride = mesh_config.config.transform_stride;

    vec3 mesh_position = vec3(
        input_transforms.transforms[gl_GlobalInvocationID.x * trf_stride + 3],
        input_transforms.transforms[gl_GlobalInvocationID.x * trf_stride + 7],
        input_transforms.transforms[gl_GlobalInvocationID.x * trf_stride + 11]
    );

    vec3 camera_pos = cull_data.data.camera_position.xyz;

    float distance_to_camera = distance(mesh_position, camera_pos);

    int lod = 0;

    for (int lod_idx = 0; lod_idx < mesh_config.config.lod_count; lod_idx++) {
        if (lod_configs.configs[lod_idx].begin_distance < distance_to_camera) {
            lod = lod_idx;
        }
    }

    atomicAdd(lod_configs.configs[lod].instance_count, 1);

    return lod;
}

void run_culling(int p_choosen_lod) {
    if (gl_GlobalInvocationID.x >= mesh_config.config.instance_count) {
        return;
    }

    int trf_stride = mesh_config.config.transform_stride;

    if (bool(cull_data.data.shadow_pass) || IsVisible(gl_GlobalInvocationID.x)) {
        int transform_idx = atomicAdd(lod_configs.configs[p_choosen_lod].current_index, 1);

        for (int i = 0; i < 12; i++) {
            output_transforms.transforms[transform_idx * trf_stride + i] = input_transforms.transforms[gl_GlobalInvocationID.x * trf_stride + i];
        }

        for (int i = 0; i < lod_configs.configs[p_choosen_lod].surface_count; i++) {
            if (lod_configs.configs[p_choosen_lod].surfaces[i] != -1) {
                atomicAdd(indirect.commands[lod_configs.configs[p_choosen_lod].surfaces[i]].instance_count, 1);
            }
        }
    }
}

void main() {
    if (gl_GlobalInvocationID.x == 0) {
        atomicExchange(indirect.commands[0].instance_count, 0);
        for (int lod_idx = 0; lod_idx < mesh_config.config.lod_count; lod_idx++) {
            atomicExchange(lod_configs.configs[lod_idx].instance_count, 0);
        }
        atomicExchange(last_transform_batch_idx, 0);
    }

    barrier();

    int lod = run_lod_selection();

    barrier();

    // Write commands and reset LODs
    if (gl_GlobalInvocationID.x < mesh_config.config.lod_count) {
        int batch_idx = atomicAdd(last_transform_batch_idx, lod_configs.configs[gl_GlobalInvocationID.x].instance_count);
        lod_configs.configs[gl_GlobalInvocationID.x].current_index = batch_idx;

        int surface_count = lod_configs.configs[gl_GlobalInvocationID.x].instance_count;
        for (int i = 0; i < lod_configs.configs[gl_GlobalInvocationID.x].surface_count; i++) {
            int surf = lod_configs.configs[gl_GlobalInvocationID.x].surfaces[i];
            if (surf == -1) {
                continue;
            }

            indirect.commands[surf].instance_count = 0;
            indirect.commands[surf].first_instance = batch_idx;
        }
    }

    barrier();

    run_culling(lod);
}
