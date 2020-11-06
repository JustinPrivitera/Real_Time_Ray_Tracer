#pragma once

#include "shape.h"
// #include "../mat.h"
#include "triangle.h"
#include "../math/ray.h"
#include "axis_aligned_box.h"
#include "pigment.h"

#define OBJ_ID 6

using namespace std;

class obj : public shape
{
public:
	obj(vector<triangle> tris, pigment p) 
		: triangles(tris), 
		  bound(axis_aligned_box()), 
		  most_recently_used(0),
		  name("obj"),
		  shape{p}
	{
		setup(tris);
	}
	obj(vector<triangle> tris, pigment p, string name) 
		: triangles(tris), 
		  bound(axis_aligned_box()), 
		  most_recently_used(0),
		  name(name),
		  shape{p}
	{
		setup(tris);
	}
	obj() 
		: triangles(vector<triangle>()), 
		  bound(axis_aligned_box()), 
		  most_recently_used(0),
		  name("obj"),
		  shape{pigment()}
	{
		// setup(tris);
	}

	virtual double eval_ray(ray* r) override;
	virtual vec3 compute_normal() const override;
	virtual string whoami() const override;
	virtual axis_aligned_box bounding_box() const;
	void realign_triangles(vector<triangle> tris);
	void setup(vector<triangle> tris)
	{
		realign_triangles(tris);
		bound = bounding_box();
	}

	virtual int id() const override
	{
		return OBJ_ID;
	}

public:
	vector<triangle> triangles;
	axis_aligned_box bound;
	int most_recently_used; // index of the most recently used triangle
	string name;
};

void obj::realign_triangles(vector<triangle> tris) // move the triangles to around the origin
{
	triangles.clear();
	vec3 avgs = vec3();
	double size = tris.size();
	for (int i = 0; i < size; i ++)
		avgs += tris[i].avg_pt();
	avgs = avgs / size;
	for (int i = 0; i < size; i ++)
		triangles.push_back(
			triangle(tris[i].a - avgs, tris[i].b - avgs, tris[i].c - avgs, tris[i].normal, tris[i].p));
}

axis_aligned_box obj::bounding_box() const
{
	// cout << "here" << endl;
	vec3 max = triangles[0].a;
	vec3 min = triangles[0].a;
	for (int i = 0; i < triangles.size(); i ++)
	{
		vec3 vert0 = triangles[i].a;
		vec3 vert1 = triangles[i].b;
		vec3 vert2 = triangles[i].c;
		vec3 currmax = vec3(max3(vert0.x, vert1.x, vert2.x), max3(vert0.y, vert1.y, vert2.y),
			max3(vert0.z, vert1.z, vert2.z));
		vec3 currmin = vec3(min3(vert0.x, vert1.x, vert2.x), min3(vert0.y, vert1.y, vert2.y),
			min3(vert0.z, vert1.z, vert2.z));
		max = maxv(max, currmax);
		min = minv(min, currmin);
	}
	return axis_aligned_box(min, max, pigment());
}

double obj::eval_ray(ray* r) 
{
	double box_t = bound.eval_ray(r);
	if (box_t < 0) // we're not in the bounding box
		return -1;
	// so we are in the bounding box
	double t, res_t;
	int index = -1;
	t = -1;
	for (int i = 0; i < triangles.size(); i ++)
	{
		res_t = triangles[i].eval_ray(r);
		if (res_t > 0)
		{
			if (res_t < t || t < 0)
			{
				t = res_t;
				index = i;
			}
		}
	}
	if (index == -1) // we were in bounding box but did not hit any triangles
		return -1;
	most_recently_used = index; // this is set for normal computation... see below
	return t; // we hit a triangle
}

vec3 obj::compute_normal() const {
	return triangles[most_recently_used].normal;
}

string obj::whoami() const {
	return "obj";
}
