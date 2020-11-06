#pragma once

#include <glm/gtc/type_ptr.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// using namespace std;
using namespace glm;

class pigment {
public:
	pigment() : rgb(vec3()) {}
	pigment(vec3 v) : rgb(v) {}
public:
	vec3 rgb;
};