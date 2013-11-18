#include "Point.h"

Point::Point()
{
	position.zero();
	velocity.zero();
	force = Vec3(0, -10, 0);
	mass = 1.0f;
	damping = 0.99f;
	steps = 0;
	printf("world\n");
}

Point::Point(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
	velocity.zero();
	force = Vec3(0, -1, 0);
	mass = 1.0f;
	damping = 0.99f;
	steps = 0;
}


Point::~Point(void)
{
}

void Point::clearForce()
{
	force.zero();
}

void Point::integrateVelocity()
{
	/*XMVECTOR forceV = XMLoadFloat3(&force);
	XMVECTOR velocityV = XMLoadFloat3(&velocity);
	XMVectorScale(forceV, 1.0f/mass);
	XMVectorAdd(velocityV, forceV);
	force = X*/

	Vec3 deltaVelocity = 1.0f / mass * (force - (damping * velocity)) * timeStep;
	velocity += deltaVelocity;
}

void Point::integratePosition()
{
	position += velocity * timeStep;

	if (position.y < -0.5f)
		position.y = -0.5f;
}

void Point::step()
{
	steps++;
	if (position.y <= -0.5f && velocity.y < 0)
		velocity.y *= -1;
	integrateVelocity();
	integratePosition();
	printf("%d %f %f\n", steps, velocity.y, position.y);
	printf("%f, %f\n", timeStep, mass);
	printf("%f, %f, %f\n", force.x, force.y, force.z);
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


void Point::draw()
{

}

