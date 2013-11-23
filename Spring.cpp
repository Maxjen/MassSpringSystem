#include "Spring.h"

Spring::Spring(void)
{
}

Spring::Spring(vector<Point>* points, int point1, int point2, float stiffness, float initialLength)
{
	this->points = points;
	this->point1 = point1;
	this->point2 = point2;
	this->stiffness = stiffness;
	this->initialLength = initialLength;
}

Spring::~Spring(void)
{
}

void Spring::addElasticForces()
{
	Vec3 p1Pos = (*points)[point2].getPosition();
	Vec3 p2Pos = (*points)[point1].getPosition();
	//float currentLength = distance(p1Pos, p2Pos);
	Vec3 d = p2Pos - p1Pos;
	float currentLength = sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
	Vec3 normVec = p2Pos - p1Pos;
	normVec.normalize();
	Vec3 force = -stiffness * (currentLength - initialLength) * normVec;
	(*points)[point1].addForce(force.x, force.y, force.z);
	(*points)[point2].addForce(-force.x, -force.y, -force.z);
}

void Spring::addElasticForcesMidPoint()
{
	Vec3 p1Pos = (*points)[point2].getPositionTmp();
	Vec3 p2Pos = (*points)[point1].getPositionTmp();
	//float currentLength = distance(p1Pos, p2Pos);
	Vec3 d = p2Pos - p1Pos;
	float currentLength = sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
	Vec3 normVec = p2Pos - p1Pos;
	normVec.normalize();
	Vec3 force = -stiffness * (currentLength - initialLength) * normVec;
	(*points)[point1].addForce(force.x, force.y, force.z);
	(*points)[point2].addForce(-force.x, -force.y, -force.z);
}

int Spring::getPoint1()
{
	return point1;
}

int Spring::getPoint2()
{
	return point2;
}
