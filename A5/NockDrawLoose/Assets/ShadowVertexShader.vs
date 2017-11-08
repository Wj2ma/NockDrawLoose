#version 330

// Taken from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping

in vec3 position;

uniform mat4 mvp;

void main() {
	gl_Position = mvp * vec4(position, 1.0);
}
