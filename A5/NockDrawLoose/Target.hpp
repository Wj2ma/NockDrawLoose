#pragma once

#include <glm/glm.hpp>
#include <AL/al.h>
#include "GeometryNode.hpp"

class Target {
public:
	Target(GeometryNode* n, glm::vec3 pos, ALuint hS, ALuint wS, ALuint bS,
		glm::vec3 rotation = glm::vec3(), float cT = 0.0f, glm::vec3 deltaPos = glm::vec3());
	~Target();

	void nextFrame(float elapsedTime);
	// arrowNode is used to translate back to the intersection point if it does intersect.
	bool intersects(GeometryNode* arrowNode, glm::vec3 point, glm::vec3 direction,
		int& bullseyes);

	GeometryNode* node;
	bool inTransition;
	bool shouldDelete;

private:
	float cycleTime; // In ms.
	float totalTime; // In ms.
	float angle;
	glm::vec4 positions[3];

	ALuint hitSound;
	ALuint woodSound;
	ALuint bullseyeSound;
};
