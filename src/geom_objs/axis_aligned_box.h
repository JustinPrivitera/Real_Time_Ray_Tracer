#pragma once

#include "shape.h"
#include "rectangle.h"

#define AA_BOX_ID 7

// rectangular prism on the axis
class axis_aligned_box : public shape
{
public:
	axis_aligned_box(vec3 blc, vec3 trc, pigment p) : blc(blc), trc(trc), shape{p} {
		// build the faces
		// cout << blc << endl;
		// cout << trc << endl;
		set_faces();
		most_recently_hit = 0;
	}
	axis_aligned_box() : blc(vec3(0,0,0)), trc(vec3(2,2,2)), shape{pigment()} 
	{
		set_faces();
		most_recently_hit = 0;
	}

	virtual double eval_ray(ray* r) override;
	virtual string whoami() const override;
	virtual vec3 compute_normal() const override;
	void set_faces()
	{
		faces[0] = axis_aligned_rectangle(
			blc, vec3(0, trc.y - blc.y, 0), vec3(0, 0, trc.z - blc.z), p);
		faces[1] = axis_aligned_rectangle(
			blc + vec3(0, trc.y - blc.y, 0), vec3(trc.x - blc.x, 0, 0), vec3(0, 0, trc.z - blc.z), p);
		faces[2] = axis_aligned_rectangle(
			blc + vec3(trc.x - blc.x, trc.y - blc.y, 0), 
			vec3(0, -1 * (trc.y - blc.y), 0), vec3(0, 0, trc.z - blc.z), p);
		faces[3] = axis_aligned_rectangle(
			blc + vec3(trc.x - blc.x, 0, 0), vec3(-1 * (trc.x - blc.x), 0, 0), vec3(0, 0, trc.z - blc.z), p);
		faces[4] = axis_aligned_rectangle(
			blc + vec3(0, 0, trc.z - blc.z), vec3(0, trc.y - blc.y, 0), vec3(trc.x - blc.x, 0, 0), p);
		faces[5] = axis_aligned_rectangle(
			blc + vec3(trc.x - blc.x, trc.y - blc.y, 0), 
			vec3(-1 * (trc.x - blc.x), 0, 0), vec3(0, -1 * (trc.y - blc.y), 0), p);
	}

	vector<vec3> get_corners() const
	{
		vector<vec3> corners;
		corners.push_back(blc);
		corners.push_back(vec3(0, trc.y - blc.y, 0) + blc);
		corners.push_back(vec3(trc.x - blc.x, trc.y - blc.y, 0) + blc);
		corners.push_back(vec3(trc.x - blc.x, 0, 0) + blc);
		corners.push_back(trc);
		corners.push_back(vec3(0, blc.y - trc.y, 0) + trc);
		corners.push_back(vec3(blc.x - trc.x, blc.y - trc.y, 0) + trc);
		corners.push_back(vec3(blc.x - trc.x, 0, 0) + trc);
		return corners;
	}

	virtual int id() const override
	{
		return AA_BOX_ID;
	}

public:
	vec3 blc; // bottom left corner
	vec3 trc; // top right corner
	axis_aligned_rectangle faces[6];
	int most_recently_hit;
};

double axis_aligned_box::eval_ray(ray* r) 
{
	double min = -1;
	int index = -1;
	for (int i = 0; i < 6; i ++)
	{
		double t = faces[i].eval_ray(r);
		if (t > 0 && min < 0)
		{
			min = t;
			index = i;
		}
		else if (t > 0 && t < min && min > 0)
		{
			min = t;
			index = i;
		}
	}
	most_recently_hit = index;
	return min;
}

vec3 axis_aligned_box::compute_normal() const {
	if (most_recently_hit != -1)
		return faces[most_recently_hit].normal;
	cerr << "bad normal computation for axis_aligned_box" << endl;
	return vec3(0,0,1);

	// if (pos.z > blc.z - 0.001 && pos.z < blc.z + 0.001)
	// 	return faces[5].normal;
	// if (pos.z > trc.z - 0.001 && pos.z < trc.z + 0.001)
	// 	return faces[4].normal;
	// if (pos.x > blc.x - 0.001 && pos.x < blc.x + 0.001)
	// 	return faces[0].normal;
	// if (pos.x > trc.x - 0.001 && pos.x < trc.x + 0.001)
	// 	return faces[2].normal;
	// if (pos.y > blc.y - 0.001 && pos.y < blc.y + 0.001)
	// 	return faces[3].normal;
	// if (pos.y > trc.y - 0.001 && pos.y < trc.y + 0.001)
	// 	return faces[1].normal;
	// // cout << pos << endl;
	
}

string axis_aligned_box::whoami() const {
	return "axis_aligned_box";
}
