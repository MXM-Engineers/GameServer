#pragma once
#include "sokol_gfx.h"
#include <glm/mat4x4.hpp>

inline sg_shader_desc& ShaderBaseMeshShaded()
{
	static sg_shader_desc desc = {0};
	desc.attrs[0].sem_name = "POSITION";
	desc.attrs[1].sem_name = "NORMAL";
	desc.attrs[1].sem_index = 1;

	desc.vs.uniform_blocks[0].size = sizeof(glm::mat4) * 2;
	desc.vs.uniform_blocks[1].size = sizeof(glm::vec4);

	desc.vs.source =
	R"RAW(
		cbuffer params: register(b0)
		{
			float4x4 vp;
			float4x4 model;
		};
		cbuffer params: register(b1)
		{
			float4 color;
		};

		struct vs_in
		{
			float4 pos: POSITION;
			float4 normal: NORMAL1;
		};

		struct vs_out
		{
			float4 color: COLOR0;
			float4 normal: NORMAL0;
			float4 pos: SV_Position;
		};

		vs_out main(vs_in inp)
		{
			vs_out outp;
			outp.pos = mul(mul(vp, model), inp.pos);
			outp.color = color;
			outp.normal = float4(mul(inp.normal.xyz, (float3x3)model), 0);
			return outp;
		};
	)RAW";

	desc.fs.source =
	R"RAW(
		float4 main(float4 color: COLOR0, float4 normal4: NORMAL0): SV_Target0
		{
			float3 normal = normalize(normal4).xyz;
			float3 sunDir = normalize(float3(5, 5, -10));
			float diffuse = max(0, dot(-sunDir, normal));
			return color * diffuse;
		}
	)RAW";

	return desc;
}

inline sg_shader_desc& ShaderLine()
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
