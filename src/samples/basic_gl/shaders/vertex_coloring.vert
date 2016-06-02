#version 400 core

layout(location=0) in vec2 VertexPosition;

uniform vec3        VertexColor;

// layout              uniform mat3x2      Rotation;

layout(binding = 0) uniform transforms {
    layout mat3 rotation;
};

layout(location=0) out vec3 VertexOutputColor;

void main() {
    const vec3 tf_pos = rotation * vec3(VertexPosition, 1.0f);

    gl_Position = vec4(tf_pos, 1.0f);
    VertexOutputColor = VertexColor;
}
