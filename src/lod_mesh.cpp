#include "lod_mesh.h"

Ref<Mesh> LODMesh::generate_mesh(const int p_mesh_quality) {
    PackedVector3Array vertices;
    PackedVector2Array uvs;
    const int vertices_per_side = p_mesh_quality + 2;

    // Calculate total vertices: original plane + skirt vertices
    // Skirt adds 4 corner vertices and 4 sides (each with vertices_per_side - 2 vertices, excluding corners)
    int total_vertices = (vertices_per_side * vertices_per_side) + 4 + 4 * (vertices_per_side - 2);
    vertices.resize(total_vertices);
    uvs.resize(total_vertices);

    // Generate original plane vertices and UVs
    for (int x = 0; x < vertices_per_side; x++) {
        const float x_coord = x / (float)(vertices_per_side - 1);
        for (int y = 0; y < vertices_per_side; y++) {
            const int idx = y * vertices_per_side + x;
            const float y_coord = y / (float)(vertices_per_side - 1);
            vertices[idx] = Vector3(x_coord, 0.0, y_coord);
            uvs[idx] = Vector2(x_coord, y_coord);
        }
    }

    // Add skirt vertices
    int skirt_vertex_start = vertices_per_side * vertices_per_side;
    int current_vertex = skirt_vertex_start;

    // Helper function to add skirt vertex
    auto add_skirt_vertex = [&](float x, float z, float u, float v) {
        vertices[current_vertex] = Vector3(x, -1.0, z);
        uvs[current_vertex] = Vector2(u, v);
        return current_vertex++;
    };

    // Add skirt vertices for each side, ensuring no duplicate corners
    // Bottom side (y = 0)
    for (int x = 0; x < vertices_per_side; x++) {
        if (x == 0 || x == vertices_per_side - 1) continue; // Skip corners
        float x_coord = x / (float)(vertices_per_side - 1);
        add_skirt_vertex(x_coord, 0.0, x_coord, 0.0);
    }

    // Top side (y = 1)
    for (int x = 0; x < vertices_per_side; x++) {
        if (x == 0 || x == vertices_per_side - 1) continue; // Skip corners
        float x_coord = x / (float)(vertices_per_side - 1);
        add_skirt_vertex(x_coord, 1.0, x_coord, 1.0);
    }

    // Left side (x = 0)
    for (int y = 0; y < vertices_per_side; y++) {
        if (y == 0 || y == vertices_per_side - 1) continue; // Skip corners
        float y_coord = y / (float)(vertices_per_side - 1);
        add_skirt_vertex(0.0, y_coord, 0.0, y_coord);
    }

    // Right side (x = 1)
    for (int y = 0; y < vertices_per_side; y++) {
        if (y == 0 || y == vertices_per_side - 1) continue; // Skip corners
        float y_coord = y / (float)(vertices_per_side - 1);
        add_skirt_vertex(1.0, y_coord, 1.0, y_coord);
    }

    // Add corner vertices
    int corner_bl = add_skirt_vertex(0.0, 0.0, 0.0, 0.0); // Bottom-left
    int corner_br = add_skirt_vertex(1.0, 0.0, 1.0, 0.0); // Bottom-right
    int corner_tl = add_skirt_vertex(0.0, 1.0, 0.0, 1.0); // Top-left
    int corner_tr = add_skirt_vertex(1.0, 1.0, 1.0, 1.0); // Top-right

    PackedInt32Array indices;

    // Generate original plane indices
    for (int x = 0; x < vertices_per_side - 1; x++) {
        for (int y = 0; y < vertices_per_side - 1; y++) {
            const int top_left_idx = y * vertices_per_side + x;
            const int top_right_idx = y * vertices_per_side + x + 1;
            const int bottom_right_idx = (y + 1) * vertices_per_side + x + 1;
            const int bottom_left_idx = (y + 1) * vertices_per_side + x;

            indices.push_back(top_left_idx);
            indices.push_back(top_right_idx);
            indices.push_back(bottom_left_idx);

            indices.push_back(top_right_idx);
            indices.push_back(bottom_right_idx);
            indices.push_back(bottom_left_idx);
        }
    }

    // Add skirt indices with corrected winding order
    // Bottom side (y = 0)
    for (int x = 0; x < vertices_per_side - 1; x++) {
        int top_idx = x;
        int bottom_idx = (x == 0) ? corner_bl :
                        (x == vertices_per_side - 1) ? corner_br :
                        skirt_vertex_start + x - 1;
        int next_top_idx = x + 1;
        int next_bottom_idx = (x == vertices_per_side - 2) ? corner_br :
                             skirt_vertex_start + x;

        // Reversed winding order
        indices.push_back(bottom_idx);
        indices.push_back(next_top_idx);
        indices.push_back(top_idx);

        indices.push_back(bottom_idx);
        indices.push_back(next_bottom_idx);
        indices.push_back(next_top_idx);
    }

    // Top side (y = 1)
    int top_row_start = skirt_vertex_start + (vertices_per_side - 2);
    for (int x = 0; x < vertices_per_side - 1; x++) {
        int top_idx = (vertices_per_side - 1) * vertices_per_side + x;
        int bottom_idx = (x == 0) ? corner_tl :
                        (x == vertices_per_side - 1) ? corner_tr :
                        top_row_start + x - 1;
        int next_top_idx = top_idx + 1;
        int next_bottom_idx = (x == vertices_per_side - 2) ? corner_tr :
                             top_row_start + x;

        // Reversed winding order
        indices.push_back(top_idx);
        indices.push_back(next_top_idx);
        indices.push_back(bottom_idx);

        indices.push_back(next_top_idx);
        indices.push_back(next_bottom_idx);
        indices.push_back(bottom_idx);
    }

    // Left side (x = 0)
    int left_row_start = top_row_start + (vertices_per_side - 2);
    for (int y = 0; y < vertices_per_side - 1; y++) {
        int top_idx = y * vertices_per_side;
        int bottom_idx = (y == 0) ? corner_bl :
                        (y == vertices_per_side - 1) ? corner_tl :
                        left_row_start + y - 1;
        int next_top_idx = (y + 1) * vertices_per_side;
        int next_bottom_idx = (y == vertices_per_side - 2) ? corner_tl :
                             left_row_start + y;

        // Reversed winding order
        indices.push_back(top_idx);
        indices.push_back(next_top_idx);
        indices.push_back(bottom_idx);

        indices.push_back(next_top_idx);
        indices.push_back(next_bottom_idx);
        indices.push_back(bottom_idx);
    }

    // Right side (x = 1)
    int right_row_start = left_row_start + (vertices_per_side - 2);
    for (int y = 0; y < vertices_per_side - 1; y++) {
        int top_idx = y * vertices_per_side + (vertices_per_side - 1);
        int bottom_idx = (y == 0) ? corner_br :
                        (y == vertices_per_side - 1) ? corner_tr :
                        right_row_start + y - 1;
        int next_top_idx = (y + 1) * vertices_per_side + (vertices_per_side - 1);
        int next_bottom_idx = (y == vertices_per_side - 2) ? corner_tr :
                             right_row_start + y;

        // Reversed winding order
        indices.push_back(bottom_idx);
        indices.push_back(next_top_idx);
        indices.push_back(top_idx);

        indices.push_back(bottom_idx);
        indices.push_back(next_bottom_idx);
        indices.push_back(next_top_idx);
    }

    PackedFloat32Array dummy_tangents;
    dummy_tangents.resize(vertices.size() * 4);
    dummy_tangents.fill(0.0f);

    Array mesh_arr;
    mesh_arr.resize(ArrayMesh::ARRAY_MAX);
    mesh_arr[ArrayMesh::ARRAY_VERTEX] = vertices;
    mesh_arr[ArrayMesh::ARRAY_INDEX] = indices;
    mesh_arr[ArrayMesh::ARRAY_TEX_UV] = uvs;
    mesh_arr[ArrayMesh::ARRAY_TANGENT] = dummy_tangents;
    Ref<ArrayMesh> pl_mesh;
    pl_mesh.instantiate();
    pl_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mesh_arr);

    return pl_mesh;
}
