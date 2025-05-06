#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec4 positions[]; };
layout(std430, binding = 1) buffer Vel { vec4 velocities[]; };

uniform vec3 cubeMin;
uniform vec3 cubeMax;
uniform float restitution;  // e.g., 0.2 = loses 80% of energy
uniform uint numParticles;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    vec3 pos = positions[i].xyz;
    vec3 vel = velocities[i].xyz;

    bool inside = all(greaterThanEqual(pos, cubeMin)) && all(lessThanEqual(pos, cubeMax));

    if (inside) {
        vec3 cubeCenter = 0.5 * (cubeMin + cubeMax);
        vec3 dir = normalize(pos - cubeCenter);

        // Resolve by pushing out along the closest face
        float dx = min(abs(pos.x - cubeMin.x), abs(pos.x - cubeMax.x));
        float dy = min(abs(pos.y - cubeMin.y), abs(pos.y - cubeMax.y));
        float dz = min(abs(pos.z - cubeMin.z), abs(pos.z - cubeMax.z));

        if (dx <= dy && dx <= dz) {
            if (pos.x > cubeCenter.x) {
                pos.x = cubeMax.x;
                vel.x = abs(vel.x) * restitution;
            } else {
                pos.x = cubeMin.x;
                vel.x = -abs(vel.x) * restitution;
            }
        } else if (dy <= dz) {
            if (pos.y > cubeCenter.y) {
                pos.y = cubeMax.y;
                vel.y = abs(vel.y) * restitution;
            } else {
                pos.y = cubeMin.y;
                vel.y = -abs(vel.y) * restitution;
            }
        } else {
            if (pos.z > cubeCenter.z) {
                pos.z = cubeMax.z;
                vel.z = abs(vel.z) * restitution;
            } else {
                pos.z = cubeMin.z;
                vel.z = -abs(vel.z) * restitution;
            }
        }

        positions[i].xyz = pos;
        velocities[i].xyz = vel;
    }
}