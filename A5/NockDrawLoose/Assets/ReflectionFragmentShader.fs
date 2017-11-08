#version 330 core
out vec4 fragColour;

in vec3 normal;
in vec3 position;
in vec4 shadowPosition;

uniform vec3 cameraPos;
uniform vec3 diffuse;
uniform samplerCube skybox;
uniform sampler2D shadowMap;

uniform int disableShadows;

void main()
{
		vec3 shadowPos = shadowPosition.xyz / shadowPosition.w;
    shadowPos = shadowPos * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, shadowPos.xy).r;
    float visibility = 1.0;

    // Direction from fragment to light source.
    if (disableShadows == 0) {
      if (shadowPos.z - 0.0001 > closestDepth) {
          visibility = 0.5;
      }
    }

    vec3 I = normalize(position - cameraPos);
  	vec3 R = reflect(I, normalize(normal));
    fragColour = vec4(visibility * diffuse * texture(skybox, R).rgb, 1.0);
}