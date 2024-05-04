#version 450 core

layout (binding = 0) uniform sampler2D DIFFUSE_MAP;

struct DirectionalLight {
	vec4 ka;
	vec4 kd;
	vec4 ks;
	vec3 direction;
	vec3 halfVector;
	float shininess;
	float strength;
};

layout (binding = 1) uniform LightData {
	DirectionalLight SceneLights[8];
	uint NumLights;
};

in VS_OUT_FS_IN {
	vec3 N; // normal in view space
	vec3 V; // view vector (from point to eye)
	vec3 P; // point position in view space
	vec2 TexCoord;
} fsIn;

layout (location = 0) out vec4 FinalFragColor;

void main() {
	vec4 clr = vec4(0);
	const vec4 mtl = texture(DIFFUSE_MAP, fsIn.TexCoord);

	for (uint i = 0; i < NumLights; ++i) {
		const DirectionalLight thisLight = SceneLights[i];
		// ambient (La * Ka)
		clr += thisLight.ka * mtl.a;

		// diffuse
		// dot(N, L) * Ld * Kd
		clr += max(0.0, dot(fsIn.N, thisLight.direction)) * mtl * thisLight.kd;

		// specular
		const vec3 R = normalize(reflect(-thisLight.direction, fsIn.N));
		clr += pow(max(0.0, dot(R, fsIn.V)), thisLight.shininess) * mtl
			* thisLight.ks
			* thisLight.strength
			;
	}

	FinalFragColor = clamp(clr, 0.0, 1.0);

	// FinalFragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
