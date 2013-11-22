#pragma once

#include "Vec3.h"
#include <cstdio>

//DirectX includes
#include <DirectXMath.h>
using namespace DirectX;

class Point
{
private:
	Vec3 position;
	Vec3 velocity;
	Vec3 gravityForce;
	Vec3 externalForce;
	float mass;
	float damping;
	float timeStep;
	bool isFixed;
public:
	Point();
	Point(float x, float y, float z);
	~Point(void);

	void clearForce();
	void addForce(float x, float y, float z);
	void integrateVelocity();
	void integratePosition();
	void step();

	void translate(float x, float y, float z);

	void setIsFixed(bool isFixed);
	bool getIsFixed();
	void setPosition(float x, float y, float z);
	Vec3 getPosition();
	void setTimeStep(float timeStep);
	void setMass(float mass);
};

