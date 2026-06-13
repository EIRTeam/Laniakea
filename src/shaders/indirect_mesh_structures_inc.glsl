#define MAX_SURFACE_COUNT 4

struct IndirectMeshLODConfig {
    int surface_count;
    int surfaces[4];
    int transform_stride;
    float begin_distance;
    int current_index;
    int instance_count;
};

struct IndirectMeshCulledInstance {
    int instance_idx;
    int lods[4];
};

struct IndirectMeshCommandBufferCommand {
    int index_count;
    int instance_count;
    int first_index;
    int vertex_offset;
    int first_instance;
};

struct IndirectMeshConfig {
    int instance_count;
    int lod_count;
    int culled_instances_count;
    int transform_stride;
    float radius;
};

struct CullData {
    float P00;                    // offset 0 (4 bytes)
    float P11;                    // offset 4 (4 bytes)
    float znear;                  // offset 8 (4 bytes)
    float zfar;                   // offset 12 (4 bytes)
    float frustum_planes[4];      // offset 16 (16 bytes)
    vec4 camera_position;         // offset 32 (16 bytes)
    mat4 view;                    // offset 48 (64 bytes)
    int shadow_pass;              // offset 116 (4 bytes)
    int padding[3];
};
