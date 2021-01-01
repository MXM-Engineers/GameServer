#pragma once
#include "sokol_gfx.h"
#include <glm/mat4x4.hpp>

inline sg_shader_desc& ShaderBaseShaded()
{
	static sg_shader_desc desc = {0};
	desc.attrs[0].sem_name = "POSITION";
	desc.attrs[1].sem_name = "COLOR";
	desc.attrs[1].sem_index = 1;

	desc.vs.uniform_blocks[0].size = sizeof(glm::mat4);

	desc.vs.source =
	R"RAW(
		cbuffer params: register(b0)
		{
		   float4x4 mvp;
		};

		struct vs_in
		{
		   float4 pos: POSITION;
		   float4 color: COLOR1;
		};

		struct vs_out
		{
		   float4 color: COLOR0;
		   float4 pos: SV_Position;
		};

		vs_out main(vs_in inp)
		{
		   vs_out outp;
		   outp.pos = mul(mvp, inp.pos);
		   outp.color = inp.color;
		   return outp;
		};
	)RAW";

	desc.fs.source =
	R"RAW(
		float4 main(float4 color: COLOR0): SV_Target0
		{
			return color;
		}
	)RAW";

	return desc;
}
