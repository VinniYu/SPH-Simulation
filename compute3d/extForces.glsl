#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec4 positions[]; };
layout(std430, binding = 1) buffer Vel { vec4 velocities[]; };

uniform float dt;
uniform float gravity;

uniform vec3 boundsMin;
uniform vec3 boundsMax;
const float damping = 1.0;

void checkBoundary(uint i) {
    vec3 pos = positions[i].xyz;
    vec3 vel = velocities[i].xyz;

    for (int j = 0; j < 3; j++) {
        if (pos[j] < boundsMin[j]) {
            pos[j] = boundsMin[j];
            vel[j] *= -damping;
        }
        if (pos[j] > boundsMax[j]) {
            pos[j] = boundsMax[j];
            vel[j] *= -damping;
        }
    }

    positions[i].xyz = pos;
    velocities[i].xyz = vel;
}

void main()
{
    uint i = gl_GlobalInvocationID.x;

    // apply gravity
    velocities[i].y -= gravity * dt;    

    positions[i].xyz += velocities[i].xyz * dt;

    checkBoundary(i);
}
