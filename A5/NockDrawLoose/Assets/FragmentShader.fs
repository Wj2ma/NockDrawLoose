#version 330

struct LightSource {
    vec3 position;
    vec3 rgbIntensity;
};

in VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
    vec2 uv;
    vec4 shadowPosition;
	LightSource light;
} fs_in;

out vec4 fragColour;

struct Material {
    vec3 kd;
    vec3 ks;
    float shininess;
};
uniform Material material;

// Ambient light intensity for each RGB component.
uniform vec3 ambientIntensity;

uniform sampler2D textureSampler;
uniform sampler2D shadowMap;

uniform int disableTransparency;
uniform int disableShadows;
uniform int disableTextures;

vec4 phongModel(vec3 fragPosition, vec3 fragNormal, vec2 uv, float visibility) {
	LightSource light = fs_in.light;

    // Direction from fragment to light source.
    vec3 l = normalize(light.position - fragPosition);

    // Direction from fragment to viewer (origin - fragPosition).
    vec3 v = normalize(-fragPosition.xyz);

    float n_dot_l = max(dot(fragNormal, l), 0.0);

	vec3 diffuse;
	diffuse = material.kd * n_dot_l;

    vec3 specular = vec3(0.0);

    if (n_dot_l > 0.0) {
		// Halfway vector.
		vec3 h = normalize(v + l);
        float n_dot_h = max(dot(fragNormal, h), 0.0);

        specular = material.ks * pow(n_dot_h, material.shininess);
    }

    if (disableTextures == 0) {
        vec4 textureColour = texture(textureSampler, uv);
        float alpha;
        if (disableTransparency == 0) {
            alpha = textureColour.a;
        } else {
            alpha = 1.0;
        }
        return vec4(ambientIntensity + visibility * light.rgbIntensity * (diffuse * textureColour.rgb + specular), alpha);
    } else {
        return vec4(ambientIntensity + visibility * light.rgbIntensity * (diffuse + specular), 1.0);
    }
}

void main() {
    vec3 shadowPos = fs_in.shadowPosition.xyz / fs_in.shadowPosition.w;
    shadowPos = shadowPos * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, shadowPos.xy).r;
    float visibility = 1.0;

    // Direction from fragment to light source.
    if (disableShadows == 0) {
        vec3 l = normalize(fs_in.light.position - fs_in.position_ES);
        float cosTheta = clamp(dot(fs_in.normal_ES, l), 0.0, 1.0);
        float bias = clamp(0.000005 * tan(acos(cosTheta)), 0.0, 0.0001);

        if (shadowPos.z - bias > closestDepth) {
            visibility = 0.5;
        }
    }

    fragColour = phongModel(fs_in.position_ES, fs_in.normal_ES, fs_in.uv, visibility);
}
