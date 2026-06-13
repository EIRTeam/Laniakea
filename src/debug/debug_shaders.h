#pragma once

const char *debug_shader = R"(
    shader_type spatial;
    render_mode unshaded, cull_disabled;

    instance uniform vec4 color : source_color;

    void vertex() {
        #ifdef POINT
        POINT_SIZE = 4.0;
        #endif
    }

    void fragment() {
        ALBEDO.rgb = color.rgb;
        ALPHA = color.a;
    }
)";

const char *debug_shader_nodepth = R"(
    shader_type spatial;
    render_mode unshaded, depth_test_disabled, cull_disabled;

    instance uniform vec4 color : source_color;

    void vertex() {
        #ifdef POINT
        POINT_SIZE = 4.0;
        #endif
    }

    void fragment() {
        ALBEDO.rgb = color.rgb;
        ALPHA = color.a;
    }
)";

const char *debug_shader_point = R"(
    shader_type spatial;
    #define POINT 1;
    render_mode unshaded;

    instance uniform vec4 color : source_color;

    void vertex() {
        #ifdef POINT
        POINT_SIZE = 4.0;
        #endif
    }

    void fragment() {
        ALBEDO.rgb = color.rgb;
        ALPHA = color.a;
    }
)";
