#pragma once

#include "shape.h"

#define RECTANGLE_ID 3
// #define AA_RECTANGLE_ID 4

class rectangle : public shape
{
public:
	rectangle(vec3 llv, vec3 right, vec3 up, vec3 p) : 
		llv(llv), right(right), up(up), shape{p} {
			normal = normalize(cross(right, up));
		}
	rectangle() : llv(vec3()), normal(vec3()), right(vec3()), up(vec3()) {}

public:
	vec3 llv; // lower left vertex
	vec3 normal;
	vec3 right;
	vec3 up;
};
