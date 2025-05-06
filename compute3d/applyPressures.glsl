#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec4 positions[]; };
layout(std430, binding = 1) buffer Vel { vec4 velocities[]; };
layout(std430, binding = 5) buffer Pressures { float pressures[]; };

uniform float dt;
uniform float restDensity;
uniform float smoothingRadius;
uniform float stiffness;
uniform uint numParticles;
uniform vec3 boundsMin;
uniform vec3 boundsMax;
const float damping = 1.0;

const float PI = 3.14159265359;

// Gradient of smoothing kernel
vec3 smoothingKernelGradient(vec3 r, float h) {
    float r_len = length(r);
    if (r_len >= h || r_len == 0.0) return vec3(0.0);

    float coeff = -45.0 / (PI * pow(h, 6.0));
    return coeff * pow(h - r_len, 2.0) * normalize(r);
}

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
    if (i >= numParticles) return;

    vec3 xi = positions[i].xyz;
    float pi = pressures[i];

    vec3 pressureForce = vec3(0.0);

    for (uint j = 0; j < numParticles; ++j)
    {
        if (i == j) continue;

        vec3 xj = positions[j].xyz;
        float pj = pressures[j];

        vec3 r = xi - xj;
        vec3 gradW = smoothingKernelGradient(r, smoothingRadius);

        // equation 4 from paper
        pressureForce += -(pi + pj) / (restDensity * restDensity) * gradW * stiffness;
    }

    // update velocity and positions
    velocities[i].xyz += dt * pressureForce;
    positions[i].xyz  += velocities[i].xyz * dt;

    checkBoundary(i);
}
