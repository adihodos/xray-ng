#version 440 core

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 FragColor;

out vec4 FinalFragColor;

void main() {
    FinalFragColor = vec4((FragPos + FragColor) * 0.5f, 1.0f);
}