#include "Primitive.hpp"
#include "polyroots.hpp"
#include <iostream>
#include <glm/gtx/io.hpp>
#include <glm/ext.hpp>

using namespace std;
using namespace glm;

Primitive::~Primitive()
{
}

vec3 Primitive::getPosition() {
	return vec3();
}

// Idea taken from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
double Primitive::boxIntersection(vec3 pos, vec3 maxPos, vec3 lookFrom, vec3 direction) {
	double minT;
	double maxT;

	double tx[2];
	getT(pos.x, lookFrom.x, direction.x, maxPos.x, tx);
	double ty[2];
	getT(pos.y, lookFrom.y, direction.y, maxPos.y, ty);

	if (tx[0] > ty[1] || ty[0] > tx[1]) {
		return -1.0f;
	}
	minT = std::max(tx[0], ty[0]);
	maxT = std::min(tx[1], ty[1]);

	double tz[2];
	getT(pos.z, lookFrom.z, direction.z, maxPos.z, tz);

	if (minT > tz[1] || tz[0] > maxT) {
		return -1.0f;
	}
	minT = std::max(minT, tz[0]);

	if (minT < 0) {
		minT = std::min(maxT, tz[1]);
	}

	return minT;
}

void Primitive::getT(
	float posD, float lookFromD, float directionD, double maxPosD, double t[2]) {
	if (directionD >= 0.0f) {
		t[0] = (posD - lookFromD);
		t[1] = (maxPosD - lookFromD);
		if (directionD == 0.0f) {
			t[0] = t[0] > 0 ? INT_MAX : INT_MIN;
			t[1] = t[1] > 0 ? INT_MAX : INT_MIN;
		} else {
			t[0] /= directionD;
			t[1] /= directionD;
		}
	} else {
		t[0] = (maxPosD - lookFromD) / directionD;
		t[1] = (posD - lookFromD) / directionD;
	}
}


Sphere::~Sphere()
{
}

float Sphere::intersects(vec3 lookFrom, vec3 direction) {
	return -1.0f; // Not needed.
}

Cube::~Cube()
{
}

float Cube::intersects(vec3 lookFrom, vec3 direction) {
	return -1.0f; // Not needed.
}

vec3 Cube::updateNormal(vec3 point) {
	float epsilon = 0.001f;

	// Front face.
	if (epsilon >= point[2] && -epsilon <= point[2]) {
		return vec3(0.0f, 0.0f, -1.0f);
	}
	// Back face.
	else if (epsilon + 1 >= point[2]
		&& -epsilon + 1 <= point[2]) {
		return vec3(0.0f, 0.0f, 1.0f);
	}
	// Left face.
	else if (epsilon >= point[0]
		&& -epsilon <= point[0]) {
		return vec3(-1.0f, 0.0f, 0.0f);
	}
	// Right face.
	else if (epsilon + 1 >= point[0]
		&& -epsilon + 1 <= point[0]) {
		return vec3(1.0f, 0.0f, 0.0f);
	}
	// Bottom face.
	else if (epsilon >= point[1]
		&& -epsilon <= point[1]) {
		return vec3(0.0f, -1.0f, 0.0f);
	}
	// Top face.
	else if (epsilon + 1 >= point[1]
		&& -epsilon + 1 <= point[1]) {
		return vec3(0.0f, 1.0f, 0.0f);
	}

	return normalize(point);
}

NonhierSphere::~NonhierSphere()
{
}

float NonhierSphere::intersects(vec3 lookFrom, vec3 direction) {
	return -1.0f; // Not needed.
}

vec3 NonhierSphere::getPosition() {
	return m_pos;
}

NonhierBox::~NonhierBox()
{
}

float NonhierBox::intersects(vec3 lookFrom, vec3 direction) {
	return -1.0f; // Not needed.
}

vec3 NonhierBox::updateNormal(vec3 point) {
	float epsilon = 0.001f;

	// Front face.
	if (m_pos[2] + epsilon >= point[2] && m_pos[2] - epsilon <= point[2]) {
		return vec3(0.0f, 0.0f, -1.0f);
	}
	// Back face.
	else if (m_pos[2] + epsilon + m_size >= point[2]
		&& m_pos[2] - epsilon + m_size <= point[2]) {
		return vec3(0.0f, 0.0f, 1.0f);
	}
	// Left face.
	else if (m_pos[0] + epsilon >= point[0]
		&& m_pos[0] - epsilon <= point[0]) {
		return vec3(-1.0f, 0.0f, 0.0f);
	}
	// Right face.
	else if (m_pos[0] + epsilon + m_size >= point[0]
		&& m_pos[0] - epsilon + m_size <= point[0]) {
		return vec3(1.0f, 0.0f, 0.0f);
	}
	// Bottom face.
	else if (m_pos[1] + epsilon >= point[1]
		&& m_pos[1] - epsilon <= point[1]) {
		return vec3(0.0f, -1.0f, 0.0f);
	}
	// Top face.
	else if (m_pos[1] + epsilon + m_size >= point[1]
		&& m_pos[1] - epsilon + m_size <= point[1]) {
		return vec3(0.0f, 1.0f, 0.0f);
	}

	return normalize(point - m_pos);
}