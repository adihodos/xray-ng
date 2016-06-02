#version 440 core
#pragma debug(on)
#pragma optimize(off)

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec2 VertexTexCoord;

layout (row_major) uniform LightSettings {
    vec3    LightPos;
    vec3    LightDir;
    float   LightIntensity;
};

layout (row_major) uniform Transforms {
    mat4    Model;
    mat4    View;
    mat4    Projection;
    mat4    Normal;
};

uniform vec3 VertexColor;

layout (location = 0) out vec3 FragPos;
layout (location = 1) out vec3 FragColor;

void main() {
    const vec3 worldPos = (Model * vec4(VertexPosition, 1.0f)).xyz;
    const vec3 normalViewPos = (View * Normal * vec4(VertexNormal, 0.0f)).xyz;

    gl_Position = Projection * vec4(worldPos, 1.0f);

    FragPos = normalize(normalViewPos + LightIntensity * (LightPos - LightDir));
    FragColor = vec3(LightIntensity * VertexTexCoord, 1.0f) + VertexColor;
}
