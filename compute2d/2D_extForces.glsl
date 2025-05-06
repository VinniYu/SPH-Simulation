#version 430

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Pos { vec2 positions[]; };
layout(std430, binding = 1) buffer Vel { vec2 velocities[]; };

uniform float dt;
uniform float gravity;

uniform float mouseX;
uniform float mouseY;
uniform float mouseStrength;
uniform float mouseRadius;
uniform int isDown;
uniform int forceType;

void main()
{
    uint i = gl_GlobalInvocationID.x;

    // apply gravity
    velocities[i].y -= gravity * dt;

    if (isDown == 1) {
        vec2 mousePos = vec2(mouseX, mouseY);
        vec2 dir = positions[i] - mousePos;
        float dist = length(dir);

        if (dist < mouseRadius && dist > 1e-5) {
            vec2 pushDir = normalize(dir);
            float falloff = 1.0 - (dist / mouseRadius);
            velocities[i] += forceType * pushDir * falloff * mouseStrength * dt;
        }
    }

    positions[i] += velocities[i] * dt;
}
