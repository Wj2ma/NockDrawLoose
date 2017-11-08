#version 330 core
// These shaders are inspired by https://learnopengl.com/#!Advanced-OpenGL/Cubemaps
in vec3 originalPos;
in vec3 originalNorm;

out vec3 normal;
out vec3 position;
out vec4 shadowPosition;

uniform mat4 model;
uniform mat4 pv;

uniform mat4 depthMVP;

void main()
{
    normal = mat3(transpose(inverse(model))) * originalNorm;
    position = vec3(model * vec4(originalPos, 1.0));
    shadowPosition = depthMVP * vec4(originalPos, 1.0);
    gl_Position = pv * model * vec4(originalPos, 1.0);
}