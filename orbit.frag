#version 330 core
out vec4 FragColor;

uniform vec3 lineColor = vec3(0.5, 0.8, 1.0); // ����������� ���� �� ���������

void main()
{
    FragColor = vec4(lineColor, 1.0);
}