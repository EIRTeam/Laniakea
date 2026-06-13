#pragma once

#include "chunkinator/chunkinator_chunk.h"
#include "chunkinator/chunkinator_layer.h"

class RandomPointLayer : public ChunkinatorLayer {
public:
	struct RandomPointGenerationSettings {
		int seed = 0;
		Vector2i grid_element_count;
		float jitter_factor = 1.0f;
		int chunk_size = 16384;
	};

private:
	RandomPointGenerationSettings generation_settings;

public:
	virtual int get_chunk_size() const override;
	virtual Ref<ChunkinatorChunk> instantiate_chunk() override;
	Vector<Vector2> get_points_in_bounds(Rect2 p_world_bounds) const;
	void set_settings(const RandomPointGenerationSettings &p_settings);
};

class RandomPointChunk : public ChunkinatorChunk {
	LocalVector<Vector2> points;
	RandomPointLayer::RandomPointGenerationSettings generation_settings;

public:
	virtual void generate() override;
	virtual void debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const override;
	friend class RandomPointLayer;
};
