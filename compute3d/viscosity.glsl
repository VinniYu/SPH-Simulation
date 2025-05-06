#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec4 positions[]; };
layout(std430, binding = 1) buffer Vel { vec4 velocities[]; };

uniform float viscosityStrength;
uniform float smoothingRadius;
uniform uint numParticles;

const float PI = 3.14159265359;

float viscosityKernel(float r, float h) {
    if (r >= h) return 0.0;
    
    float diff = h - r;
    float coeff = 15.0 / (2.0 * PI * pow(h, 3.0)); // Poly6-style normalizing factor
    return coeff * diff;
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    vec3 xi = positions[i].xyz;
    vec3 vi = velocities[i].xyz;

    vec3 viscosityForce = vec3(0.0);

    for (uint j = 0; j < numParticles; j++) {
        if (i == j) continue;

        vec3 xj = positions[j].xyz;
        vec3 vj = velocities[j].xyz;

        float dst = length(xi - xj);
        float influence = viscosityKernel(dst, smoothingRadius);
        viscosityForce += (vj - vi) * influence;
    }

    velocities[i].xyz += viscosityStrength * viscosityForce;
}