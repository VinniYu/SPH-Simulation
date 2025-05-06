#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec2 positions[]; };
layout(std430, binding = 1) buffer Vel { vec2 velocities[]; };

uniform vec2 boxMin;
uniform vec2 boxMax;
uniform float restitution;  // e.g., 0.2 = loses 80% of energy
uniform uint numParticles;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    vec2 pos = positions[i];
    vec2 vel = velocities[i];

    bool inside = all(greaterThanEqual(pos, boxMin)) && all(lessThanEqual(pos, boxMax));

    if (inside) {
        vec2 boxCenter = 0.5 * (boxMin + boxMax);
        
        // Find closest wall
        float dx = min(abs(pos.x - boxMin.x), abs(pos.x - boxMax.x));
        float dy = min(abs(pos.y - boxMin.y), abs(pos.y - boxMax.y));

        if (dx <= dy) {
            if (pos.x > boxCenter.x) {
                pos.x = boxMax.x;
                vel.x = abs(vel.x) * restitution;
            } else {
                pos.x = boxMin.x;
                vel.x = -abs(vel.x) * restitution;
            }
        } else {
            if (pos.y > boxCenter.y) {
                pos.y = boxMax.y;
                vel.y = abs(vel.y) * restitution;
            } else {
                pos.y = boxMin.y;
                vel.y = -abs(vel.y) * restitution;
            }
        }

        positions[i] = pos;
        velocities[i] = vel;
    }
}
