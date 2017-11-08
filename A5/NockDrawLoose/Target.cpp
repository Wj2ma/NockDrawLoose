#include "Target.hpp"
#include "globals.hpp"

#include <iostream>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>

using namespace std;
using namespace glm;

Target::Target(GeometryNode* n, vec3 pos, ALuint hS, ALuint wS, ALuint bS, vec3 rotation,
	float cT, vec3 deltaPos) : node(n), hitSound(hS), woodSound(wS), bullseyeSound(bS),
	cycleTime(cT), totalTime(0.0f), inTransition(true), shouldDelete(false), angle(0.0f) {
			node->rotate('x', 90.0f * rotation.x);
			node->rotate('y', 90.0f * rotation.y);
			node->rotate('z', 90.0f * rotation.z);
			node->translate(pos);
			node->setInit();
			positions[0] = node->get_transform()[3];
			positions[1] = positions[0] + vec4(deltaPos, 0.0f);
			positions[2] = positions[0];
}

Target::~Target() {
	delete node;
}

void Target::nextFrame(float elapsedTime) {
	totalTime += elapsedTime;
	mat4 newTransform = node->get_initTrans();

	// Rotate target in place or out.
	if (inTransition) {
		float deltaAngle = elapsedTime * 90.0f / TARGET_TRANS_TIME;
		if (shouldDelete && angle > 0.0f) {
			angle -= deltaAngle;
			if (angle <= 0.0f) {
				angle = 0.0f;
				inTransition = false;
			}
		} else if (angle < 90.0f) {
			angle += deltaAngle;
			if (angle >= 90.0f) {
				angle = 90.0f;
				inTransition = false;
			}
		}
	} else if (cycleTime > 0.0f) {
		// Key frame animation of moving target.
		float framePos = fmod(totalTime, cycleTime) * 2.0f / cycleTime;
		vec4 prevFrame = positions[(int) floor(framePos)];
		vec4 nextFrame = positions[(int) ceil(framePos)];
		vec4 newPos = prevFrame + (framePos - floor(framePos)) * (nextFrame - prevFrame);
		newTransform[3] = newPos;
	}

	node->set_transform(newTransform);
	node->setInit();
	node->rotate('x', angle);
}

bool Target::intersects(GeometryNode* arrowNode, vec3 point, vec3 direction,
	int& bullseyes) {
	mat4 invTransform = inverse(node->get_transform());
	vec3 localPoint = vec3(invTransform * vec4(point, 1.0f));
	vec3 localDir = vec3(invTransform * vec4(direction, 0.0f));
	vec3 normalizedLocalDir = normalize(localDir);
	float t = node->m_primitive->intersects(localPoint, normalizedLocalDir);
	float maxT = length(localDir);
	if (t >= 0.0f && t <= maxT) {
		// Translate to where it should be intersected.
		arrowNode->translate(
			vec3(node->get_transform() * vec4(t * normalizedLocalDir, 0.0f)));
		vec3 position = vec3(arrowNode->get_transform()[3]);
		// Position for sound. Scale down by 2 because it is too quiet.
		float pos[] = { position[0] / 2, position[1] / 2, position[2] / 2 };
		// Check if it actually hit the target, and not the stem.
		vec3 newPoint = localPoint + t * normalizedLocalDir;
		// Hardcoded values for where the target circle is locally.
		if (newPoint.x <= 0.5 && newPoint.x >= -0.5
			&& newPoint.z <= -0.71f && newPoint.z >= -1.71f) {
			if (newPoint.x <= 0.06 && newPoint.x >= -0.06
				&& newPoint.z <= -1.15f && newPoint.z >= -1.27f) {
				alSourcefv(bullseyeSound, AL_POSITION, pos);
				alSourcePlay(bullseyeSound);
				++bullseyes;
			} else {
				alSourcefv(hitSound, AL_POSITION, pos);
				alSourcePlay(hitSound);
			}
			shouldDelete = true;
			inTransition = true;
		} else {
			alSourcefv(woodSound, AL_POSITION, pos);
			alSourcePlay(woodSound);
		}
		return true;
	}
	return false;
}