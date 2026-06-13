#include "baked_map.h"

void BakedMap::from_image(Ref<Image> p_image) {
	dimensions = p_image->get_size();
	data.resize(dimensions.x * dimensions.y);

	float *data_w = data.ptrw();

	for (int x = 0; x < dimensions.x; x++) {
		for (int y = 0; y < dimensions.y; y++) {
			data_w[x + y * dimensions.x] = p_image->get_pixel(x, y).r;
		}
	}
}

float BakedMap::sample_point(Vector2i p_point) const {
	return data[p_point.x + p_point.y * dimensions.x];
}

float BakedMap::bilinearly_sample(Vector2 p_uv) const {
	DEV_ASSERT(p_uv.x >= 0.0);

	// Convert to heightmap pixel coordinates
	double u = p_uv.x * (dimensions.x - 1);
	double v = p_uv.y * (dimensions.y - 1);

	// Bilinear interpolation for smoothness
	int u0 = (int)Math::floor(u);
	int v0 = (int)Math::floor(v);
	int u1 = MIN(u0 + 1, dimensions.x - 1);
	int v1 = MIN(v0 + 1, dimensions.y - 1);
	double u_frac = u - u0;
	double v_frac = v - v0;

	if (u0 >= dimensions.x || v0 >= dimensions.y) {
		return 0.0f;
	}

	// Sample values from the channel
	double h00 = sample_point(Vector2i(u0, v0));
	double h10 = sample_point(Vector2i(u1, v0));
	double h01 = sample_point(Vector2i(u0, v1));
	double h11 = sample_point(Vector2i(u1, v1));

	// Bilinear interpolation
	double out = h00 * (1 - u_frac) * (1 - v_frac) +
			h10 * u_frac * (1 - v_frac) +
			h01 * (1 - u_frac) * v_frac +
			h11 * u_frac * v_frac;
	return out;
}
