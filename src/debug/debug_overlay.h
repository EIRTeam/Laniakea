#pragma once

#include "console/cvar.h"
#include "debug/debug_text_line_drawer.h"
#include "godot_cpp/classes/canvas_item.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/shader_material.hpp"
#include "godot_cpp/classes/sphere_mesh.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include <optional>

using namespace godot;

class DebugOverlay {
    static DebugOverlay *singleton;
    Node3D *root_node = nullptr;
    static CVar overlays_frozen_cvar;
public:
    enum ProcessPass {
        PROCESS,
        PHYSICS
    };
private:
    struct DebugText {
        Vector3 world_pos;
        String text;
    };
    struct Overlay {
        Vector<Node3D*> nodes;
        std::optional<DebugText> debug_text;
        float end_time = 0.0f;
        ProcessPass process_pass = PROCESS;
    };

    LocalVector<Overlay> overlays;

    Ref<ShaderMaterial> debug_overlay_material;
    Ref<ShaderMaterial> debug_overlay_material_point;
    
    // Default meshes
    Ref<Mesh> sphere_mesh;
    Ref<Mesh> cylinder_mesh;
    Ref<Mesh> circle_mesh_solid;
    Ref<Mesh> circle_mesh;

    void _register_overlay(const Overlay &p_overlay);
    static MeshInstance3D *_create_mesh_instance(const Ref<Mesh> &p_mesh, const Color &p_color, const bool p_depth_test = true);
    void _dispose_overlay(int p_idx);
    DebugTextLineDrawer *text_line_drawer = nullptr;
public:
    void advance(ProcessPass p_pass);
    void initialize(SceneTree *p_main_loop);
    static DebugOverlay *get_singleton();
    static void sphere(const Vector3 &p_center, const float p_radius, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void line(const Vector3 &p_from, const Vector3 &p_to, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void cylinder(const Vector3 &p_at, const float p_height, const float p_radius, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void horz_arrow(const Vector3 &p_from, const Vector3 &p_to, const float p_width, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void vert_arrow(const Vector3 &p_from, const Vector3 &p_to, const float p_width, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void path(const PackedVector3Array p_path, bool p_draw_points, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void horz_circle(const Vector3 &p_at, const float p_radius, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void cone(const Vector3 &p_from, const Vector3 &p_to, const float p_angle, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void text(const Vector3 &p_at, const String &p_text, const Color &p_color, const bool p_depth_test = true, const float p_duration = 0.0f);
    static void mesh_with_trf(const Transform3D &p_trf, const Ref<Mesh> &p_mesh, const bool p_depth_test = true, const float p_duration = 0.0f);
    ~DebugOverlay();
};