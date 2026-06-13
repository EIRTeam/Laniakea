#include "chunkinator_test.h"
#include "chunkinator/chunkinator_debugger.h"
#include "chunkinator/chunkinator_generator.h"
#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/plane_mesh.hpp"
#include "godot_cpp/classes/shader_material.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "segment_quadtree.h"
#include "terrain_generator/random_point_scatter.h"
#include "terrain_generator/scatter.h"
#include "terrain_generator/scatter_manager.h"
#include "terrain_generator/terrain_chunk_node.h"
#include "terrain_generator/terrain_heightmap.h"
#include "terrain_generator/terrain_heightmap_combine_layer.h"
#include "terrain_generator/terrain_roads.h"
#include "lod_mesh.h"
#include "terrain_generator/terrain_settings.h"

void ChunkinatorTest::_on_generation_completed() {
    /*for (int i = 0; i < modified_heightmap_layer->get_chunk_count(); i++) {
        Ref<TerrainRoadConnectionChunk> chunk = modified_heightmap_layer->get_chunk(i);
        if (superchunk_meshes.has(chunk->get_chunk_index())) {
            continue;
        }
        TerrainChunkNode *node = memnew(TerrainChunkNode);
        add_child(node);
        Ref<ImageTexture> heightmap_texture = ImageTexture::create_from_image(chunk->get_heightmap());
        node->initialize(modified_heightmap_layer->get_chunk_size(), chunk->get_chunk_index(), chunk->get_heightmap(), heightmap_texture, plane_mesh);
        node->update();
        superchunk_meshes.insert(chunk->get_chunk_index(), node);
    }*/
}
void ChunkinatorTest::_input(const Ref<InputEvent> &p_event) {
    if (Ref<InputEventKey> ev = p_event; ev.is_valid()) {
        if (ev->is_pressed() && !ev->is_echo() && ev->get_keycode() == godot::KEY_F2) {
            chunkinator->set_generation_rect(Rect2(-30000, -30000, 20000, 20000));
            chunkinator->generate();
        }
    }
}

void ChunkinatorTest::_bind_methods() {

}

void ChunkinatorTest::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {

            Ref<TerrainSettings> settings = ResourceLoader::get_singleton()->load("res://terrain_settings.tres");
            plane_mesh = LODMesh::generate_mesh(settings->get_mesh_quality());


            chunkinator.instantiate();
            manager = memnew(TerrainManager);
            manager->set_terrain_settings(settings);
            add_child(manager);
            Ref<TerrainHeightmapLayer> Heightmap_Layer;
            Ref<RandomPointLayer> B_Layer;
            Ref<TerrainRoadConnectionLayer> Road_connection_Layer;
            Ref<TerrainFinalCombineLayer> heightmap_combine_layer;
            Ref<ChunkinatorGeneratorLayer> terrain_generator_layer;
            Ref<ChunkinatorGeneratorLayer> scatter_generator_layer;

            Heightmap_Layer.instantiate();
            B_Layer.instantiate();
            Road_connection_Layer.instantiate();
            heightmap_combine_layer.instantiate();

            const StringName heightmap_layer_name = "Heightmap Layer";
            const StringName heightmap_combine_layer_name = "Heightmap Combine Layer";
            const StringName b_layer_name = "Road Random Points";
            const StringName road_connection_layer_name = "Road Connections";
            const StringName terrain_generator_layer_name = "Generator Terrain";
            const StringName scatter_generator_layer_name = "Generator Scatter";

            terrain_generator_layer = ChunkinatorGeneratorLayer::create(4096, DRAW_DISTANCE);
            scatter_generator_layer = ChunkinatorGeneratorLayer::create(256, settings->get_scatter_generation_radius());

            chunkinator->insert_layer(heightmap_layer_name, Heightmap_Layer);
            heightmap_combine_layer->set_heightmap_size(settings->get_geometry_chunk_heightmap_size());
            chunkinator->insert_layer(b_layer_name, B_Layer);
            chunkinator->insert_layer(road_connection_layer_name, Road_connection_Layer);
            chunkinator->insert_layer(heightmap_combine_layer_name, heightmap_combine_layer);
            chunkinator->insert_layer(terrain_generator_layer_name, terrain_generator_layer);
            chunkinator->insert_layer(scatter_generator_layer_name, scatter_generator_layer);
            //chunkinator->insert_layer("C Layer", C_Layer);
            //chunkinator->insert_layer("D Layer", D_Layer);

            chunkinator->add_layer_dependency(heightmap_layer_name, road_connection_layer_name, Vector2i(50, 50));
            chunkinator->add_layer_dependency(b_layer_name, road_connection_layer_name, Vector2i(3000, 3000));
            chunkinator->add_layer_dependency(road_connection_layer_name, heightmap_combine_layer_name, Vector2i(1, 1));
            chunkinator->add_layer_dependency(heightmap_combine_layer_name, terrain_generator_layer_name, Vector2i());
            //chunkinator->add_layer_dependency(a_layer_name, "C Layer", Vector2i());
            //chunkinator->add_layer_dependency(b_layer_name, "D Layer", Vector2i());
            //chunkinator->add_layer_dependency("C Layer", "D Layer", Vector2i());

            modified_heightmap_layer = Road_connection_Layer;

            for (int i = 0; i < settings->get_scatter_layer_count(); i++) {
                Ref<TerrainScatterLayerSettings> scatter_layer_settings = settings->get_scatter_layer(i);

                Ref<RandomPointLayer> point_layer;
                point_layer.instantiate();
                point_layer->set_settings({
                    .grid_element_count = Vector2i(scatter_layer_settings->get_layer_element_count_per_side(), scatter_layer_settings->get_layer_element_count_per_side()),
                    .chunk_size = scatter_layer_settings->get_layer_chunk_size()
                });

                const StringName point_layer_name = String(scatter_layer_settings->get_debug_name()) + " Points";

                chunkinator->insert_layer(point_layer_name, point_layer);

                Ref<ScatterLayer> worldgen_layer;
                worldgen_layer.instantiate();

                worldgen_layer->initialize(scatter_layer_settings, point_layer);

                chunkinator->insert_layer(scatter_layer_settings->get_debug_name(), worldgen_layer);

                chunkinator->add_layer_dependency(point_layer_name, scatter_layer_settings->get_debug_name(), Vector2i());
                chunkinator->add_layer_dependency(heightmap_layer_name, point_layer_name, Vector2i());
                chunkinator->add_layer_dependency(scatter_layer_settings->get_debug_name(), scatter_generator_layer_name, Vector2i());
            }

            ScatterManager *scatter_manager = memnew(ScatterManager);
            scatter_manager->set_chunkinator(chunkinator);
            add_child(scatter_manager);

            manager->set_chunkinator(chunkinator);
            chunkinator->build();
            chunkinator->set_generation_rect(Rect2(-DRAW_DISTANCE, -DRAW_DISTANCE, DRAW_DISTANCE*2, DRAW_DISTANCE*2));
            chunkinator->generate();
            Window *w = memnew(Window);
            add_child(w);
            w->hide();
            w->set_force_native(false);
            w->show();
            w->set_size(Vector2(512, 512));
            debugger = memnew(ChunkinatorDebugger);
            w->add_child(debugger);
            debugger->set_chunkinator(chunkinator);
            debugger->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

            chunkinator->connect("generation_completed", callable_mp(this, &ChunkinatorTest::_on_generation_completed));

            Window *qt_window = memnew(Window);
            SegmentQuadTreeDebug *qtd = memnew(SegmentQuadTreeDebug);
            qt_window->add_child(qtd);
            add_child(qt_window);
            qtd->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
        } break;
        case NOTIFICATION_PROCESS: {
            Camera3D *cam = get_viewport()->get_camera_3d();

            if (chunkinator->get_generation_state() == Chunkinator::ChunkinatorGenerationState::IDLING) {
                Rect2 new_generation_rect = Rect2(-DRAW_DISTANCE, -DRAW_DISTANCE, DRAW_DISTANCE * 2, DRAW_DISTANCE * 2);
                new_generation_rect.position += Vector2(cam->get_global_position().x, cam->get_global_position().z);
                chunkinator->set_generation_rect(new_generation_rect);
                chunkinator->generate();
            } else {

                chunkinator->process_generation();

            }
            if (cam != nullptr) {
                manager->set_camera_position(cam->get_global_position());
            }
            manager->update();
        } break;
    }
}

ChunkinatorTest::ChunkinatorTest() {
    set_process(true);
}

ChunkinatorTest::~ChunkinatorTest() {
}

int ChunkinatorTestLayer::get_chunk_size() const {
    return 4096;
}

Ref<ChunkinatorChunk> ChunkinatorTestLayer::instantiate_chunk() {
    Ref<ChunkinatorTestChunk> test_chunk;
    test_chunk.instantiate();
    Ref<ChunkinatorChunk> pc = test_chunk;
    return pc;
}

void ChunkinatorTestChunk::test() {

}

void ChunkinatorTestChunk::generate() {
}

void ChunkinatorTestChunk::debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const {
    if (get_chunk_index() == Vector2i()) {
        p_debug_drawer->draw_circle(Rect2(get_chunk_bounds()).get_center());
    }
}
