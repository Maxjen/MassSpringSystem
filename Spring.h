#pragma once

#include <vector>

#include "Vec3.h"
#include "Point.h"

using namespace std;

class Spring
{
private:
	vector<Point>* points;
	int point1;
	int point2;
	float stiffness;
	float initialLength;

public:
	Spring(void);
	Spring(vector<Point>* points, int point1, int point2, float stiffness, float initialLength);
	~Spring(void);

	void addElasticForces();
	void addElasticForcesMidPoint();
	int getPoint1();
	int getPoint2();
};

