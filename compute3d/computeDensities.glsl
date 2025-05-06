#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec4 positions[]; };
layout(std430, binding = 1) buffer Vel { vec4 velocities[]; };
layout(std430, binding = 2) buffer PredPos { vec4 predPos[]; };
layout(std430, binding = 3) buffer PredVel { vec4 predVel[]; };
layout(std430, binding = 4) buffer Density { float densities[]; };
layout(std430, binding = 5) buffer Pressures { float pressures[]; };
layout(std430, binding = 10) buffer MaxDensityError { uint maxDensityError; };

uniform float dt;
uniform float restDensity;
uniform float smoothingRadius;
uniform float delta;
uniform uint numParticles;

const float PI = 3.14159265359;

// smoothing kernel, 3d poly6
float smoothingKernel(float r, float h){
    if (r >= h) return 0.0;

    float hr2 = h * h - r * r;
    float coeff = 315.0 / (64.0 * PI * pow(h, 9.0));
    return coeff * pow(hr2, 3.0);
}

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    // 1. predict vel and position in time
    predVel[i].xyz = velocities[i].xyz;
    predPos[i].xyz = positions[i].xyz + dt * predVel[i].xyz;

    // 2. predict density
    float predDensity = 0.0;
    for (uint j = 0; j < numParticles; j++) {
        // if (i == j) continue;

        float r = length(predPos[i].xyz - predPos[j].xyz);
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
