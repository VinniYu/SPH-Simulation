#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec2 positions[]; };
layout(std430, binding = 1) buffer Vel { vec2 velocities[]; };

uniform float viscosityStrength;
uniform float smoothingRadius;
uniform uint numParticles;

const float PI = 3.14159265359;

float viscosityKernel(float dst, float radius) {
    float diff = max(0.0, radius * radius - dst * dst);
    float volume = PI * pow(radius, 8.0) / 4.0;
    return (diff * diff * diff) / volume;
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    vec2 xi = positions[i];
    vec2 vi = velocities[i];

    vec2 viscosityForce = vec2(0.0);

    for (uint j = 0; j < numParticles; j++) {
        if (i == j) continue;

        vec2 xj = positions[j];
        vec2 vj = velocities[j];

        float dst = length(xi - xj);
        float influence = viscosityKernel(dst, smoothingRadius);
        viscosityForce += (vj - vi) * influence;
    }

    velocities[i] += viscosityStrength * viscosityForce;
}