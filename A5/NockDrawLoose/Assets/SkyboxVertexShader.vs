#version 330

in vec3 position;

out vec3 textureDir;

uniform mat4 pv;

void main() {
	textureDir = position;
	gl_Position = (pv * vec4(position, 1.0)).xyww;
}