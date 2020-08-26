#pragma once
#include <common/base.h>

struct Vec3
{
	f32 x, y, z;

	Vec3(){}
	Vec3(f32 x_, f32 y_, f32 z_):
		x(x_), y(y_), z(z_) {}
};
