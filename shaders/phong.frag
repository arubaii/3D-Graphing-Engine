#version 410 core

in vec2 TexCoord;
in vec3 Dir;
in vec3 Normal;
in vec3 FragPos;

layout (location = 0) out vec4 FragColor;


uniform bool useColor;
uniform bool useTexture2D;
uniform bool useTexture3D;

uniform vec4 u_Color;
uniform samplerCube u_Cubemap;
uniform sampler2D u_Texture;

uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;



void main()
{
    vec3 normal = normalize(Normal);
    vec3 lightDirection = normalize(lightPos - FragPos);
    float diff = max(dot(normal, lightDirection), 0.0);
    float lightIntensity = 1.2f;
    float ambient = 0.0;

    float specular = 0.0f;

    float specularLight = 0.50f;
    vec3 viewDirection = normalize(cameraPos - FragPos);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8);
    specular = specAmount * specularLight;


    vec4 baseColor = vec4(0.9, 0.0, 0.9, 1.0); // default magenta
    if (useColor)
    baseColor = u_Color;
    else if (useTexture2D)
    baseColor = texture(texture_diffuse1, TexCoord);

    /* ---------- Buggy, else if block, fix later */
    //    else if (useTexture3D)
    //        baseColor = texture(u_Cubemap, normalize(Dir));
    else
    baseColor = vec4(0.9, 0.0, 0.9, 1.0);


    FragColor = baseColor * lightColor * (diff + ambient + specular) * lightIntensity;
}