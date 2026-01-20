#version 410 core

layout (location = 0) out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec3 vWorldPos;

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec4 lightColor;

uniform vec4 u_Color;

uniform vec3 u_BoxMin;
uniform vec3 u_BoxMax;

void main()
{
    if (vWorldPos.x < u_BoxMin.x || vWorldPos.x > u_BoxMax.x ||
    vWorldPos.y < u_BoxMin.y || vWorldPos.y > u_BoxMax.y ||
    vWorldPos.z < u_BoxMin.z || vWorldPos.z > u_BoxMax.z)
    discard;

    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - FragPos);

    vec3 baseColor = N * 0.5 + 0.5;

    float ambient = 0.85;

    vec3 hemiDir = normalize(vec3(0.3, 1.0, 0.4));
    float diffuse = max(dot(N, hemiDir), 0.0) * 0.25;

    vec3 R = reflect(-V, N);
    float spec = pow(max(dot(V, R), 0.0), 48.0);
    float specular = spec * 0.35;

    vec3 color =
    baseColor * (ambient + diffuse) +
    specular * lightColor.rgb;

    FragColor = vec4(color, 1.0);
}