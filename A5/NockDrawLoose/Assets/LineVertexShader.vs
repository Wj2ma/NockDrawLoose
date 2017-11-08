#version 330

// Model-Space coordinates
in vec3 position;

uniform mat4 pv;

void main() {
	gl_Position = pv * vec4(position, 1);
}
