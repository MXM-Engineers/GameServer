#pragma once
#include <common/base.h>
#include <EASTL/array.h>

enum class HorizontalMoveType: u8
{
	Any,
	Stand,
	Front,
	Left,
	Right,
	Back
};

struct HorizontalMotionVariant
{
	static constexpr u32 MaxSamples = 32;

	HorizontalMoveType moveType = HorizontalMoveType::Any;
	bool hasRandom = false;
	u16 randomCaseValue = 0;
	f32 horizonRotate = 0;
	f32 duration = 0;
	u8 sampleCount = 0;
	eastl::array<f32,MaxSamples> samples{};

	f32 Sample(f32 elapsed) const
	{
		ASSERT(duration > 0);
		ASSERT(sampleCount >= 1);
		if(elapsed <= 0) return samples.front();
		if(elapsed >= duration) return samples[sampleCount - 1];
		if(sampleCount == 1) return samples.front();
		const f32 sample = elapsed / duration * (f32)(sampleCount - 1);
		const u32 index = (u32)sample;
		if(index >= (u32)(sampleCount - 1)) return samples[sampleCount - 1];
		return samples[index] + (samples[index + 1] - samples[index]) * (sample - (f32)index);
	}
};

struct HorizontalMotion
{
	static constexpr u32 MaxVariants = 8;

	u8 variantCount = 0;
	eastl::array<HorizontalMotionVariant,MaxVariants> variants{};
};
