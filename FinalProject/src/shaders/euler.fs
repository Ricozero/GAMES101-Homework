#version 330 core
in vec3 vertexNormal;
out vec4 fragColor;

void main()
{
    // fragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
    fragColor = vec4(vertexNormal.x, vertexNormal.y, vertexNormal.z, 1.0f);
}