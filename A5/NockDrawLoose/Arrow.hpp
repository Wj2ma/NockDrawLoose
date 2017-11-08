#pragma once

#include <glm/glm.hpp>
#include "GeometryNode.hpp"
#include "globals.hpp"

struct PhysicsData {
	float bowMass;
	float arrowMass;
	float k;
	float gravity;

	PhysicsData() : bowMass(DEFAULT_BOW_MASS), arrowMass(DEFAULT_ARROW_MASS),
		k(DEFAULT_K), gravity(DEFAULT_GRAVITY) { }
};

class Arrow {
public:
	Arrow(GeometryNode* n, glm::vec3 dir, float drawBack, PhysicsData* p);

	glm::vec3 nextFrame(float elapsedTime);	// Returns how much the arrow translated.

	GeometryNode* node;
	glm::vec3 velocity;

private:
	PhysicsData* physics;
};
