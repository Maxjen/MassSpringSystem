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
	positionTmp = position;
	velocity.zero();
	velocityTmp.zero();
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
	externalForce += Vec3(x, y, z);
}

void Point::integrateVelocity()
{
	if (!isFixed)
	{
		Vec3 deltaVelocity = (gravityForce + externalForce - (damping * velocity)) * timeStep / mass;
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

void Point::stepEuler()
{
	if (position.y <= -0.5f && velocity.y < 0)
		velocity.y *= -1;
	integrateVelocity();
	integratePosition();
}

void Point::stepMidPoint1()
{
	if (position.y <= -0.5f && velocity.y < 0)
		velocity.y *= -1;

	if (!isFixed)
	{
		positionTmp = position + velocity * timeStep/2;
		if (positionTmp.y < -0.5f)
			positionTmp.y = -0.5f;
		velocityTmp = velocity + (gravityForce + externalForce - (damping * velocity)) * (timeStep / 2) / mass;
		position += velocityTmp * timeStep;
		if (position.y < -0.5f)
			position.y = -0.5f;
	}
}

void Point::stepMidPoint2()
{
	if (!isFixed)
		velocity += (gravityForce + externalForce - (damping * velocityTmp)) * timeStep / mass;
}

void Point::translate(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
}

void Point::setIsFixed(bool isFixed)
{
	this->isFixed = isFixed;
	if (isFixed)
	{
		positionTmp = position;
		velocity.zero();
		velocityTmp.zero();
	}
}

bool Point::getIsFixed()
{
	return isFixed;
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

Vec3 Point::getPositionTmp()
{
	return positionTmp;
}

Vec3 Point::getVelocityTmp()
{
	return velocityTmp;
}

void Point::setMass(float mass)
{
	this->mass = mass;
	gravityForce = Vec3(0, -10, 0) * mass;
}

float Point::getMass()
{
	return mass;
}
