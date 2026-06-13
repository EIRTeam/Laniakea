#include "image_sampling.h"

#include "profiling.h"

float bilinearly_sample_image_single_channel(Ref<Image> p_image, int p_channel_idx, const Vector2 &p_uv) {
	//FuncProfile;
	const Size2i image_size = p_image->get_size();

	DEV_ASSERT(p_uv.x >= 0.0);

	// Convert to heightmap pixel coordinates
	float u = p_uv.x * (image_size.x - 1);
	float v = p_uv.y * (image_size.y - 1);

	// Bilinear interpolation for smoothness
	int u0 = (int)Math::floor(u);
	int v0 = (int)Math::floor(v);
	int u1 = MIN(u0 + 1, image_size.x - 1);
	int v1 = MIN(v0 + 1, image_size.y - 1);
	float u_frac = u - u0;
	float v_frac = v - v0;

	if (u0 >= image_size.x || v0 >= image_size.y) {
		return 0.0f;
	}

	// Sample values from the channel
	float h00 = p_image->get_pixel(u0, v0)[p_channel_idx];
	float h10 = p_image->get_pixel(u1, v0)[p_channel_idx];
	float h01 = p_image->get_pixel(u0, v1)[p_channel_idx];
	float h11 = p_image->get_pixel(u1, v1)[p_channel_idx];

	// Bilinear interpolation
	float out = h00 * (1 - u_frac) * (1 - v_frac) +
			h10 * u_frac * (1 - v_frac) +
			h01 * (1 - u_frac) * v_frac +
			h11 * u_frac * v_frac;
	return out;
}
