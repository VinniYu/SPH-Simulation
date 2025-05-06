#version 430 core

layout(std430, binding = 0) buffer Pos { vec4 positions[]; };

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragWorldPos;

void main() {
    int i = gl_VertexID;

    vec4 pos = positions[gl_VertexID];
    vec4 worldPos = model * pos;
    fragWorldPos = worldPos.xyz;
    
    gl_Position = projection * view * worldPos;
    gl_PointSize = 10.0;
}
