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

layout(set = 0, binding = 4, std430) restrict buffer CullDataBuffer {
    CullData data;
}
cull_data;

layout(set = 0, binding = 5, std430) restrict buffer IndirectCommandBuffer {
    IndirectMeshCommandBufferCommand commands[];
}
indirect;

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

	//if(cull_data.data.distCull != 0)
	//{// the near/far plane culling uses camera space Z directly
	//}
    //visible = visible && center.z + radius > cull_data.data.znear && center.z - radius < cull_data.data.zfar;

	visible = visible;// || cull_data.data.cullingEnabled == 0;
	return visible;
}


void main() {
    if (gl_GlobalInvocationID.x >= mesh_config.config.instance_count) {
        return;
    }

    if (gl_GlobalInvocationID.x == 0) {
        atomicExchange(indirect.commands[0].instance_count, 0);
        //atomicExchange(mesh_config.culled_instances_count, 0);
    }

    barrier();

    if (!cull_data.data.lod_pass) {
        if ( && !IsVisible(gl_GlobalInvocationID.x)) {
            return;
        }
    }

    int idx = atomicAdd(indirect.commands[0].instance_count, 1);

    int stride = mesh_config.config.transform_stride;
    for (int i = 0; i < 12; i++) {
        output_transforms.transforms[idx * stride + i] = input_transforms.transforms[gl_GlobalInvocationID.x * stride + i];
    }

    barrier();
}
