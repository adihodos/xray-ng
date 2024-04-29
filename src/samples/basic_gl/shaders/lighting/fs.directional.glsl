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
	vec3 pos_view;
	vec3 norm_view;
	vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
	vec3 clr = vec3(0);
	const vec4 mtl = texture(DIFFUSE_MAP, fs_in.texcoord);

	for (uint i = 0; i < NumLights; ++i) {
		const DirectionalLight dl = SceneLights[i];
		float diffuse = max(0.0, dot(dl.direction, fs_in.norm_view));
		float specular = max(0.0, dot(dl.halfVector, fs_in.norm_view));

		if (diffuse == 0.0)
			specular = 0.0;
		else
			specular = pow(specular, dl.shininess);

		const vec3 scattered_light = (dl.ka + dl.kd * diffuse).xyz;
		const vec3 reflected_light = (dl.ks * specular * dl.strength).xyz;

		clr = min(mtl.rgb * scattered_light + reflected_light, vec3(1.0));
	}

	FinalFragColor = vec4(clr, mtl.a);
}
