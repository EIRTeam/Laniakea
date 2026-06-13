#pragma once

#include "chunkinator/chunkinator_chunk.h"
#include "chunkinator/chunkinator_layer.h"

class BiomeLayer : public ChunkinatorLayer {
public:
	struct RandomPointGenerationSettings {
		int seed = 0;
		int points_per_chunk = 9; // Must be a perfect square...
		float jitter_factor = 1.0f;
	};

private:
	RandomPointGenerationSettings generation_settings;

public:
	virtual int get_chunk_size() const override;
	virtual Ref<ChunkinatorChunk> instantiate_chunk() override;
	Vector<Vector2> get_points_in_bounds(Rect2 p_world_bounds) const;
};

class RandomPointChunk : public ChunkinatorChunk {
	LocalVector<Vector2> points;
	RandomPointLayer::RandomPointGenerationSettings generation_settings;

public:
	virtual void generate() override;
	virtual void debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const override;
	friend class RandomPointLayer;
};
