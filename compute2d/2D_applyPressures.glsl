#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec2 positions[]; };
layout(std430, binding = 1) buffer Vel { vec2 velocities[]; };
layout(std430, binding = 2) buffer Pressures { float pressures[]; };

uniform float smoothingRadius;
uniform float restDensity;
uniform float stiffness;
uniform float dt;
uniform uint numParticles;

const float PI = 3.14159265359;
vec2 boundsMin = vec2(0.0, 0.0);
vec2 boundsMax = vec2(1024.0, 768.0);
const float damping = 1.0;

// Gradient of smoothing kernel
vec2 smoothingKernelGradient(vec2 r, float radius)
{
    float dst = length(r);
    if (dst >= radius || dst == 0.0) return vec2(0.0);

    float volume = (PI * pow(radius, 4.0)) / 6.0;
    float coeff = -2.0 * (radius - dst) / volume;
    return coeff * normalize(r);
}


void checkBoundary(uint i)
{
    vec2 pos = positions[i];
    vec2 vel = velocities[i];

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

    positions[i] = pos;
    velocities[i] = vel;
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    vec2 xi = positions[i];
    float pi = pressures[i];

    vec2 pressureForce = vec2(0.0);

    for (uint j = 0; j < numParticles; ++j)
    {
        if (i == j) continue;

        vec2 xj = positions[j];
        float pj = pressures[j];

        vec2 r = xi - xj;
        vec2 gradW = smoothingKernelGradient(r, smoothingRadius);

        // equation 4 from paper
        pressureForce += -(pi + pj) / (restDensity * restDensity) * gradW * stiffness;
    }

    // update velocity
    vec2 newVel = velocities[i] + dt * pressureForce;
    velocities[i] = newVel;
    positions[i] += newVel * dt;

    checkBoundary(i);
}
