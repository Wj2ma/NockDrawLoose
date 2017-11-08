#include "Bow.hpp"
#include "globals.hpp"
#include <glm/gtx/transform.hpp>

#include <iostream>
#include <glm/gtx/io.hpp>

using namespace std;
using namespace glm;

Bow::Bow(GeometryNode* n, vec3 dir)
	: node(n), direction(dir), drawback(0.0f) { }

void Bow::updateDirection(float deltaX, float deltaY) {
	vec3 newDir = vec3(direction.x + deltaX, direction.y + deltaY, direction.z);
	newDir.x = clamp(newDir.x, MIN_X, MAX_X);
	newDir.y = clamp(newDir.y, MIN_Y, MAX_Y);
	newDir = normalize(newDir);

	if (newDir != direction) {
		node->set_transform(node->get_initTrans());
		vec3 initDir(0.0f, 0.0f, -1.0f);

		vec3 axis = cross(initDir, newDir);
		float angle = acos(dot(initDir, newDir));
		node->rotate(rotate(angle, axis));

		direction = newDir;
	}
}

void Bow::updateDirection(vec3 newDir) {
	newDir = normalize(newDir);
	if (newDir != direction) {
		node->set_transform(node->get_initTrans());
		vec3 initDir(0.0f, 0.0f, -1.0f);

		vec3 axis = cross(initDir, newDir);
		float angle = acos(dot(initDir, newDir));
		node->rotate(rotate(angle, axis));

		direction = newDir;
	}
}

void Bow::pull(float elapsedTime) {
	if (drawback < MAX_DRAW) {
		// Max drawback in 2 seconds.
		float deltaDraw = elapsedTime / DRAWBACK_TIME;
		deltaDraw = std::min(deltaDraw, MAX_DRAW - drawback);
		drawback += deltaDraw;
		// Pull the arrow back.
		GeometryNode* arrow = static_cast<GeometryNode*>(node->children.front());
		arrow->translate(vec3(0.0f, 0.0f, deltaDraw));
	}
}

Arrow* Bow::shoot(PhysicsData* physics) {
	// Copy the arrow from the bow and shoot it.
	GeometryNode* arrow = static_cast<GeometryNode*>(node->children.front());
	GeometryNode* shootingArrowNode = new GeometryNode(*arrow);
	Arrow* shootingArrow = new Arrow(shootingArrowNode, direction, drawback, physics);
	shootingArrowNode->set_transform(
		node->get_transform() * shootingArrowNode->get_transform());
	arrow->set_transform(arrow->get_initTrans());
	drawback = 0.0f;
	return shootingArrow;
}