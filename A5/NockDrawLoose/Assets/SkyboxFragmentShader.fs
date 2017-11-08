#version 330

out vec4 fragColour;

// https://learnopengl.com/#!Advanced-OpenGL/Cubemaps

in vec3 textureDir;
uniform samplerCube skybox;

void main() {
	fragColour = texture(skybox, textureDir);
}
