#version 330 core

in vec2 texCoord;

uniform sampler2D depthMap;

out vec4 fragColour;

void main()
{
    float depthValue = texture(depthMap, texCoord).r;
    fragColour = vec4(vec3(depthValue), 1.0);
}