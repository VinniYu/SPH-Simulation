#version 430 core

layout (location = 0) in vec3 boundPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(boundPos, 1.0);
}