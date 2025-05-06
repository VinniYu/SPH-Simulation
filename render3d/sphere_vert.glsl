#version 430 core

layout(location = 0) in vec3 vertexPosition; // vertex of the sphere mesh
layout(std430, binding = 0) buffer Pos { vec4 positions[]; }; // position of particle

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragWorldPos;
flat out int fragIndex;

void main() {
    vec3 sphereCenter = positions[gl_InstanceID].xyz;
    float radius = 0.02;

    // Transform vertex into world space
    vec3 worldPos = sphereCenter + radius * vertexPosition;
    fragWorldPos = worldPos;
    fragIndex = gl_InstanceID;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
