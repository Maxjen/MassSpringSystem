#include "Point.h"

Point::Point()
{
	position.zero();
	velocity.zero();
	externalForce.zero();
	setMass(1.0f);
	damping = 0.99f;
	isFixed = false;
}

Point::Point(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
	velocity.zero();
	externalForce.zero();
	setMass(1.0f);
	damping = 0.99f;
	isFixed = false;
}

Point::~Point(void)
{
}

void Point::clearForce()
{
	externalForce.zero();
}

void Point::addForce(float x, float y, float z)
{
	//printf("hallo, %f, %f, %f\n", x, y, z);
	externalForce += Vec3(x, y, z);
	//printf("%f, %f, %f\n", externalForce.x, externalForce.y, externalForce.z);
}

void Point::integrateVelocity()
{
	if (!isFixed)
	{
		Vec3 deltaVelocity = 1.0f / mass * (gravityForce + externalForce - (damping * velocity)) * timeStep;
		velocity += deltaVelocity;
	}
}

void Point::integratePosition()
{
	if (!isFixed)
	{
		position += velocity * timeStep;

		if (position.y < -0.5f)
			position.y = -0.5f;
	}
}

void Point::step()
{
	//printf("%f, %f, %f\n", externalForce.x, externalForce.y, externalForce.z);
	if (position.y <= -0.5f && velocity.y < 0)
		velocity.y *= -1;
	integrateVelocity();
	integratePosition();
}

void Point::setFixed()
{
	isFixed = true;
}

void Point::setPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
}

void Point::setTimeStep(float timeStep)
{
	this->timeStep = timeStep;
}

Vec3 Point::getPosition()
{
	return position;
}

void Point::setMass(float mass)
{
	this->mass = mass;
	gravityForce = Vec3(0, -10, 0) * mass;
}
