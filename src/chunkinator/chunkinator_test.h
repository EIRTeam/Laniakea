#pragma once

#include "chunkinator.h"
#include "chunkinator/chunkinator_chunk.h"
#include "chunkinator/chunkinator_debugger.h"
#include "chunkinator/chunkinator_layer.h"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "terrain_generator/terrain_chunk_node.h"
#include "terrain_generator/terrain_manager.h"

class ChunkinatorTestChunk : public ChunkinatorChunk {
	void test();
	virtual void generate() override;
	virtual void debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const override;
};

class ChunkinatorTestLayer : public ChunkinatorLayer {
public:
	int get_chunk_size() const override;
	Ref<ChunkinatorChunk> instantiate_chunk() override;
};

class ChunkinatorTest : public Node3D {
	GDCLASS(ChunkinatorTest, Node3D);
	const int DRAW_DISTANCE = 7500;
	ChunkinatorDebugger *debugger = nullptr;
	Ref<ChunkinatorLayer> modified_heightmap_layer;
	HashMap<Vector2i, TerrainChunkNode *> superchunk_meshes;
	void _on_generation_completed();
	HashMap<Vector2i, TerrainChunkNode> superchunks;
	Ref<Mesh> plane_mesh;
	TerrainManager *manager = nullptr;

public:
	virtual void _input(const Ref<InputEvent> &p_event) override;
	Ref<Chunkinator> chunkinator;
	static void _bind_methods();
	void _notification(int p_what);
	ChunkinatorTest();
	~ChunkinatorTest();
};
