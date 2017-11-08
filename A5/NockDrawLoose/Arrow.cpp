#include "Arrow.hpp"

#include <iostream>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>

using namespace std;
using namespace glm;

Arrow::Arrow(GeometryNode* n, vec3 dir, float drawBack, PhysicsData* p)
	: node(n), physics(p) {
		velocity = sqrt(physics->k * drawBack / (physics->bowMass + physics->arrowMass)) * dir;
}

vec3 Arrow::nextFrame(float elapsedTime) {
	float elapsedTimeS = elapsedTime / 1000.0f;
	vec3 translated = velocity * elapsedTimeS;
	velocity.y -= physics->gravity * elapsedTimeS;

	// Rotate arrow to its velocity direction.
	mat4 trans = node->get_transform();
	mat4 initTrans = node->get_initTrans();
	node->set_transform(mat4(initTrans[0], initTrans[1], initTrans[2], trans[3]));
	vec3 initDir(0.0f, 0.0f, -1.0f);

	vec3 normalizedV = normalize(velocity);
	vec3 axis = cross(initDir, normalizedV);
	float angle = acos(dot(initDir, normalizedV));
	node->rotate(rotate(angle, axis));
	node->rotate('z', elapsedTimeS * 100);

	return translated;
}

