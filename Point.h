#pragma once

#include "Vec3.h"
#include <cstdio>

//DirectX includes
#include <DirectXMath.h>
using namespace DirectX;

class Point
{
private:
	/*XMFLOAT3 position;
	XMFLOAT3 velocity;
	XMFLOAT3 force;*/
	Vec3 position;
	Vec3 velocity;
	Vec3 force;
	float mass;
	float damping;
	float timeStep;
	int steps;
public:
	Point();
	Point(float x, float y, float z);
	~Point(void);

	void clearForce();
	void integrateVelocity();
	void integratePosition();
	void step();

	void setPosition(float x, float y, float z);
	Vec3 getPosition();
	void setTimeStep(float timeStep);

	void draw();
};

