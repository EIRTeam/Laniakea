#pragma once

#include "godot_cpp/classes/rd_shader_source.hpp"
#include "godot_cpp/classes/rd_shader_spirv.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"

using namespace godot;

class EIRTeamShaderRD {
	BitField<RenderingDevice::ShaderStage> stages = 0;
	RID shader;

protected:
	void setup(const char *p_compute_stage) {
		Ref<RDShaderSource> shader_source;
		shader_source.instantiate();
		if (p_compute_stage != nullptr) {
			stages.set_flag(RenderingDevice::SHADER_STAGE_COMPUTE_BIT);
			shader_source->set_stage_source(RenderingDevice::SHADER_STAGE_COMPUTE, String(p_compute_stage));
		}

		RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();

		Ref<RDShaderSPIRV> spirv = rd->shader_compile_spirv_from_source(shader_source);
		for (int stage = 0; stage < RenderingDevice::SHADER_STAGE_MAX; stage++) {
			String err = spirv->get_stage_compile_error((RenderingDevice::ShaderStage)stage);
			if (err.is_empty()) {
				continue;
			}

			print_error(err);
			print_line(shader_source);
			DEV_ASSERT(false);
		}
		shader = rd->shader_create_from_spirv(spirv);
	}

public:
	RID get_shader_rid() const {
		return shader;
	}
	~EIRTeamShaderRD() {
		RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
		if (shader.is_valid()) {
			rd->free_rid(shader);
		}
	}
};
