#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec3 TexCoords;
out vec4 color;

uniform vec3 lightPos;  
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform samplerCube cubemap;

void main()
{
    float ambientStrength = 0.3f;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse 
    vec3 norm = Normal;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec4 FragColor = texture(cubemap, TexCoords);

    vec3 result = (ambient + diffuse) * FragColor;
    color = vec4(result, 1.0f);
}