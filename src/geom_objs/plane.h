#pragma once

#include "shape.h"
// #include "axis_aligned_box.h"

#define PLANE_ID 5

class plane : public shape {
public:
	plane() : shape{}, normal(vec3()), dist_from_orig(0) {
		p0 = dist_from_orig * normal;
	}
	plane(vec3 normal, float dist_from_orig, vec3 p) : 
	shape{p}, normal(normalize(normal)), dist_from_orig(dist_from_orig) {
		p0 = dist_from_orig * normal;
	}
	
	virtual string whoami() const override;

	virtual int id() const override
	{
		return PLANE_ID;
	}

public:
	vec3 normal;
	float dist_from_orig;
	vec3 p0; // point in plane
};

string plane::whoami() const {
	return "plane";
}
