#version 430 core

in vec2 fragVel;
out vec4 fragColor;

void main()
{
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5)
        discard;

    float speed = length(fragVel);
    float t = clamp(speed / 50.0, 0.0, 1.0);

    vec3 color = mix(vec3(0.1, 0.2, 1.0), vec3(0.5, 0.8, 1.0), t);

    fragColor = vec4(color, 1.0);
}
