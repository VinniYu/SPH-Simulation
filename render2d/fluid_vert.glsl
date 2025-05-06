#version 430 core

layout(std430, binding = 0) buffer PosBuffer { vec2 positions[]; };
layout(std430, binding = 1) buffer VelBuffer { vec2 velocities[]; };

uniform mat4 uProjection;

out vec2 fragVel;

void main()
{
    vec2 pos = positions[gl_VertexID];
    vec2 vel = velocities[gl_VertexID];

    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
    gl_PointSize = 6.0;

    fragVel = vel; 
}
