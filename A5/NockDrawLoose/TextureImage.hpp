#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

// Material surface properties to be used as input into a local illumination model
// (e.g. the Phong Reflection Model).
struct TextureImage {
	TextureImage() : width(0), height(0), texBufferId(0) { }

	unsigned int width;
	unsigned int height;
	std::vector<unsigned char> data;
	GLuint texBufferId;
};
