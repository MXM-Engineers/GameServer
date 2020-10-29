#pragma once
#include <common/base.h>

struct Vec2
{
	f32 x, y;

	Vec2(){}
	Vec2(f32 x_, f32 y_):
		x(x_), y(y_) {}
};

struct Vec3
{
	f32 x, y, z;

	Vec3(){}
	Vec3(f32 x_, f32 y_, f32 z_):
		x(x_), y(y_), z(z_) {}
};
