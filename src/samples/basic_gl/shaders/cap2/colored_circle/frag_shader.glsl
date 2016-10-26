#version 450 core

layout(location = 0) in vec3 OutputTexCoord;

layout(row_major, packed) uniform BlobSettings {
    vec4    InnerColor;
    vec4    OuterColor;
    float   RadiusInner;
    float   RadiusOuter;
};

const vec2 QuadCenter = vec2(0.5f, 0.5f);

out vec4 FinalFragColor;

void main() {
    const vec2 dst_vec = QuadCenter - OutputTexCoord.xy;
    const float dst = sqrt(dot(dst_vec, dst_vec));
    FinalFragColor = mix(InnerColor, OuterColor,
        smoothstep(RadiusInner, RadiusOuter, dst));
}