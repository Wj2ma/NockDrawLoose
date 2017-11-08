#pragma once

#include <glm/glm.hpp>
#include "GeometryNode.hpp"
#include "Arrow.hpp"

class Bow {
public:
	Bow(GeometryNode* n, glm::vec3 dir);

	void updateDirection(float deltaX, float deltaY);
	void updateDirection(glm::vec3 newDir);
	void pull(float elapsedTime);
	Arrow* shoot(PhysicsData* physics);

	GeometryNode* node;
	glm::vec3 direction;

private:
	float drawback;
};
