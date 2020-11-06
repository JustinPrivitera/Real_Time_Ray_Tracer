#pragma once

using namespace std;

#define NO_FINISH 0
#define REFLECTIVE 1

class finish
{
public:
	finish() : type(NO_FINISH), value(0.0) {}
	finish(int type, double value) : type(type), value(value) {}
	
public:
	int type; // 0 = no finish, 1 = reflective
	double value;
};
