#version 430 core

in vec3 fragWorldPos;
flat in int fragIndex;
out vec4 FragColor;

layout(std430, binding = 1) buffer Vel { vec4 velocities[]; };

uniform vec3 cameraPos;
uniform vec3 lightPos;

void main() {
    vec3 velocity = velocities[fragIndex].xyz;
    float speed = length(velocity);

    float speedNormalized = clamp(speed / 6.0, 0.0, 1.0); // Adjust divisor for your scale

    vec3 color = mix(vec3(0.1, 0.2, 1.0), vec3(0.5, 0.8, 1.0), speedNormalized);

    FragColor = vec4(color, 1.0);
}
