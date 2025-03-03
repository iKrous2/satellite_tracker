#version 330 core
layout (location = 0) in vec3 position;

out vec4 ourColor;

uniform mat4 transform;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); //
    ourColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
} 