#pragma once
#include <common/base.h>
#include <EASTL/array.h>

enum class JumpDirection: u8
{
	Stand,
	Front,
	Left,
	Right,
	Back,
	Count
};

struct JumpMotion
{
	f32 duration = 0;
	eastl::array<f32,11> heights{};

	f32 Sample(f32 elapsed) const
	{
		ASSERT(duration > 0);
		if(elapsed <= 0) return heights.front();
		if(elapsed >= duration) return heights.back();
		const f32 sample = elapsed / duration * 10.f;
		const u32 index = (u32)sample;
		if(index >= 10) return heights.back();
		return heights[index] + (heights[index + 1] - heights[index]) * (sample - (f32)index);
	}
};
