#version 330

// Model-Space coordinates
in vec3 position;
in vec3 normal;

in vec2 uv;

struct LightSource {
    vec3 position;
    vec3 rgbIntensity;
};
uniform LightSource light;

uniform mat4 ModelView;
uniform mat4 Perspective;

uniform mat4 depthMVP;

// Remember, this is transpose(inverse(ModelView)).  Normals should be
// transformed using this matrix instead of the ModelView matrix.
uniform mat3 NormalMatrix;

out VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
	vec2 uv;
	vec4 shadowPosition;
	LightSource light;
} vs_out;

void main() {
	vec4 pos4 = vec4(position, 1.0);

	//-- Convert position and normal to Eye-Space:
	vs_out.position_ES = (ModelView * pos4).xyz;
	vs_out.normal_ES = normalize(NormalMatrix * normal);
	vs_out.uv = uv;

	vs_out.shadowPosition = depthMVP * pos4;

	vs_out.light = light;

	gl_Position = Perspective * ModelView * vec4(position, 1.0);
}
