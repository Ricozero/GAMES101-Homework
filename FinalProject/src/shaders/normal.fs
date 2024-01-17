#version 330 core
in vec3 vertexNormal;
out vec4 fragColor;

void main()
{
    fragColor = vec4(vertexNormal, 1.0f);
}