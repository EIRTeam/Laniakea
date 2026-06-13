#pragma once

#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/texture2d_array.hpp"
#include "godot_cpp/classes/texture2d_array_rd.hpp"
#include "godot_cpp/templates/local_vector.hpp"

using namespace godot;

class TextureArrayQueue {
	RID texture_id;
	Ref<Texture2DArrayRD> texture;

public:
	struct TextureHandle {
		int texture_idx = 0;
	};

	int side_size;
	LocalVector<int> free_indices;

	void initialize(int p_size, int p_layer_count) {
		free_indices.resize(p_layer_count);
		side_size = p_size;
		for (int i = 0; i < p_layer_count; i++) {
			free_indices[i] = i;
		}

		Ref<RDTextureFormat> texture_format;
		texture_format.instantiate();
		texture_format->set_format(RenderingDevice::DATA_FORMAT_R32_SFLOAT);
		texture_format->set_width(p_size);
		texture_format->set_height(p_size);
		texture_format->set_array_layers(p_layer_count);
		texture_format->set_texture_type(RenderingDevice::TEXTURE_TYPE_2D_ARRAY);
		BitField<RenderingDevice::TextureUsageBits> usage_bits = RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT | RenderingDevice::TEXTURE_USAGE_CAN_UPDATE_BIT;
		texture_format->set_usage_bits(usage_bits);

		RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
		Ref<RDTextureView> view;
		view.instantiate();
		texture_id = rd->texture_create(texture_format, view);
		DEV_ASSERT(texture_id.is_valid());
		texture.instantiate();
		texture->set_texture_rd_rid(texture_id);
	}

	void unload_handle(const TextureHandle &p_handle) {
		DEV_ASSERT(!free_indices.has(p_handle.texture_idx));
		RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
		free_indices.push_back(p_handle.texture_idx);
	}

	TextureHandle push_texture(Ref<Image> p_image) {
		DEV_ASSERT(!free_indices.is_empty());
		int idx = free_indices[free_indices.size() - 1];
		free_indices.remove_at_unordered(free_indices.size() - 1);

		RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
		rd->texture_update(texture_id, idx, p_image->get_data());

		return TextureHandle {
			.texture_idx = idx
		};
	}

	Ref<TextureLayered> get_texture() const {
		return texture;
	}

	~TextureArrayQueue() {
		RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
		if (texture_id.is_valid()) {
			rd->free_rid(texture_id);
		}
	}
};
