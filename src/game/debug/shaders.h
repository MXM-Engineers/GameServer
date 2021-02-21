#pragma once
#include <common/base.h>
#include "sokol_gfx.h"
#include <glm/mat4x4.hpp>

struct ShaderMeshShaded
{
	struct VsUniform0
	{
		glm::mat4 viewProj;
		glm::mat4 model;
		glm::mat4 normalMat;
	};

	struct FsUniform0
	{
		glm::vec3 color;
		f32 _colorPad = 1;
		glm::vec3 sunPos;
		f32 _sunPad = 1;
		glm::vec3 test;
		f32 _testPad = 1;

		FsUniform0(glm::vec3 color_, glm::vec3 sunPos_, glm::vec3 test_)
		{
			color = color_;
			sunPos = sunPos_;
			test = test_;
		}
	};
};

inline sg_shader_desc& ShaderBaseMeshShaded()
{
	static sg_shader_desc desc = {0};
	desc.attrs[0].sem_name = "POSITION";
	desc.attrs[0].sem_index = 1;
	desc.attrs[1].sem_name = "NORMAL";
	desc.attrs[1].sem_index = 1;

	desc.vs.uniform_blocks[0].size = sizeof(ShaderMeshShaded::VsUniform0);
	desc.fs.uniform_blocks[0].size = sizeof(ShaderMeshShaded::FsUniform0);

	desc.vs.source =
	R"RAW(
		cbuffer params: register(b0)
		{
			float4x4 vp;
			float4x4 model;
			float4x4 normalMat;
		};

		struct vs_in
		{
			float4 pos: POSITION1;
			float4 normal: NORMAL1;
		};

		struct vs_out
		{
			float4 worldPos: POSITION0;
			float4 normal: NORMAL0;
			float4 pos: SV_Position;
		};

		vs_out main(vs_in inp)
		{
			vs_out outp;
			outp.pos = mul(mul(vp, model), inp.pos);
			outp.worldPos = mul(model, inp.pos);
			outp.normal = mul(normalMat, float4(inp.normal.xyz, 0));
			return outp;
		};
	)RAW";

	desc.fs.source =
	R"RAW(
		cbuffer params: register(b0)
		{
			float4 color4;
			float4 lightPos4;
			float4 test4;
		};

		float4 main(float4 worldPos4: POSITION0, float4 normal4: NORMAL0): SV_Target0
		{
			float3 color = color4.xyz;
			float3 lightPos = lightPos4.xyz;
			float3 normal = normalize(normal4.xyz);
			float3 pos = worldPos4.xyz;

			float3 lightDir = normalize(lightPos - pos);
			float diffuse = max(0, dot(lightDir, normal));
			return float4(color * diffuse, 1.0);
		}
	)RAW";

	return desc;
}

inline sg_shader_desc& ShaderBaseMeshUnlit()
{
	static sg_shader_desc desc = {0};
	desc.attrs[0].sem_name = "POSITION";
	desc.attrs[1].sem_name = "NORMAL";
	desc.attrs[1].sem_index = 1;

	desc.vs.uniform_blocks[0].size = sizeof(glm::mat4);
	desc.vs.uniform_blocks[1].size = sizeof(glm::vec3);

	desc.vs.source =
			R"RAW(
			cbuffer params: register(b0)
			{
				float4x4 mvp;
			};
			cbuffer params: register(b1)
			{
				float3 color;
			};

			struct vs_in
			{
				float4 pos: POSITION;
				float4 normal: NORMAL1;
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
				outp.color = float4(color, 1.0);
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
