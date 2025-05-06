#version 430 core

in vec3 fragWorldPos;
out vec4 FragColor;

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform mat4 view;
uniform mat4 projection;

bool intersectSphere(vec3 ro, vec3 rd, vec3 center, float radius, out vec3 hitPos, out vec3 normal) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0) return false;

    float t = -b - sqrt(h);
    if (t < 0.0) return false;

    hitPos = ro + t * rd;
    normal = normalize(hitPos - center);
    return true;
}

void main() {
    // reconstruct using screen-space coords
    vec2 ndc = gl_PointCoord * 2.0 - 1.0;

    // view space
    vec3 rayDir = normalize(vec3(ndc, -1.0));

    mat3 invView = mat3(transpose(view)); // Approximate inverse for rotation
    rayDir = normalize(invView * rayDir);

    vec3 rayOrigin = cameraPos;
    vec3 center = fragWorldPos;
    float radius = 3.0;

    vec3 hit, normal;
    if (!intersectSphere(rayOrigin, rayDir, center, radius, hit, normal)) 
        discard;

    vec3 lightDir = normalize(lightPos - hit);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = 0.4;

    vec3 baseColor = vec3(34.0/255.0, 52.0/255.0, 255.0/255.0);
    vec3 color = baseColor * (ambient + (1.0 - ambient) * diffuse);

    FragColor = vec4(color, 1.0);
}
