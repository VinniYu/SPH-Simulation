#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec2 positions[]; };
layout(std430, binding = 1) buffer Vel { vec2 velocities[]; };
layout(std430, binding = 2) buffer PredPos { vec2 predPos[]; };
layout(std430, binding = 3) buffer PredVel { vec2 predVel[]; };
layout(std430, binding = 4) buffer Pressures { float pressures[]; };
layout(std430, binding = 5) buffer Density { float densities[]; };
layout(std430, binding = 10) buffer MaxDensityError { uint maxDensityError; };

uniform float dt;
uniform float restDensity;
uniform float smoothingRadius;
uniform float delta;
uniform uint numParticles;

const float PI = 3.14159265359;

// smoothing kernel
float smoothingKernel(float dst, float radius){
    if (dst >= radius)
        return 0.0;

    float volume = (PI * pow(radius, 4.0)) / 6.0;
    float diff = (radius - dst);
    return (diff * diff) / volume;
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    // 1. predict vel and position in time
    predVel[i] = velocities[i];
    predPos[i] = positions[i] + dt * predVel[i];

    // 2. predict density
    float predDensity = 0.0;
    for (uint j = 0; j < numParticles; j++) {
        // if (i == j) continue;

        float r = length(predPos[i] - predPos[j]);
        predDensity += smoothingKernel(r, smoothingRadius); // assuming mass is 1
    }

    // save predicted density for pressure force
    densities[i] = predDensity;

    // 3. calculate density error
    float densityError = abs(predDensity - restDensity);

    // update the max density error for eta
    uint densityErrorBits = floatBitsToUint(densityError);
    atomicMax(maxDensityError, densityErrorBits);

    // 4. update pressure
    pressures[i] += delta * (predDensity - restDensity);
}
