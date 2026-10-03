#pragma once
#include <common/base.h>
#include <cmath>

struct InputClock
{
	bool initialized = false;
	f32 referenceClientTime = 0.0f;
	f64 referenceServerTime = 0.0;

	f64 ToServerTime(f32 clientTime) const
	{
		return referenceServerTime + (f64(clientTime) - referenceClientTime);
	}

	void Reset()
	{
		initialized = false;
		referenceClientTime = 0.0f;
		referenceServerTime = 0.0;
	}

	bool Observe(f32 clientTime, f64 receivedTime, f64 maximumAdvance, bool* mappingRevised = nullptr)
	{
		if(mappingRevised) *mappingRevised = false;
		if(!std::isfinite(clientTime) || !std::isfinite(receivedTime) || clientTime < 0.0f) return false;
		const f64 previousTime = initialized ? ToServerTime(clientTime) : receivedTime;
		if(!std::isfinite(previousTime) || previousTime > receivedTime + maximumAdvance) return false;
		if(!initialized || previousTime > receivedTime) {
			if(mappingRevised) *mappingRevised = initialized;
			initialized = true;
			referenceClientTime = clientTime;
			referenceServerTime = receivedTime;
		}
		return true;
	}
};
