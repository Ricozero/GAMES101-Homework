#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in float aIndex;
layout (location = 3) in vec2 aTexCoords;
struct Vertex
{
    vec3 position;
    vec3 normal;
};
layout (std430, binding = 0) buffer net
{
    Vertex vertices[];
};
out vec3 vertexPos;
out vec3 vertexNormal;
out vec2 texCoords;
out vec3 eyePos;

uniform int screenWidth;
uniform int screenHeight;
uniform mat4 view;
uniform float scale;
uniform int gpuSimulation;
uniform int indexOffset;

void main()
{
    vec3 pos, normal;
    if (bool(gpuSimulation))
    {
        Vertex v = vertices[int(aIndex) + indexOffset];
        pos = v.position;
        normal = v.normal;
    }
    else
    {
        pos = aPos;
        normal = aNormal;
    }

    // In OpenGL, camera space is left-handed, z+ pointing from camera towards front
    float aspectRatio = float(screenWidth) / screenHeight;
    float t = 400, b = -t, r = t * aspectRatio, l = -r, n = -r, f = r;
    mat4 orth = mat4(
        2/(r-l), 0, 0, -(r+l)/(r-l),
        0, 2/(t-b), 0, -(t+b)/(t-b),
        0, 0, -2/(f-n), -(f+n)/(f-n),
        0, 0, 0, 1
    );

    mat4 model = mat4(
        scale, 0, 0, 0,
        0, scale, 0, 0,
        0, 0, scale, 0,
        0, 0, 0, 1
    );

    gl_Position = orth * view * model * vec4(pos, 1.0);
    vertexPos = pos;
    vertexNormal = normal; // TODO: 加入材质后应为normalMatrix * normal
    eyePos = (view * model * vec4(0, 0, n, 1)).xyz;
}