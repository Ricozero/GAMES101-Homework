#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
out vec3 vertexPos;
out vec3 vertexNormal;
out vec2 texCoords;
out vec3 eyePos;

uniform int screenWidth;
uniform int screenHeight;
uniform mat4 view;
uniform float scale;

void main()
{
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

    gl_Position = orth * view * model * vec4(aPos, 1.0);
    vertexPos = aPos.xyz;
    vertexNormal = aNormal; // TODO: 加入材质后应为normalMatrix * aNormal
    eyePos = (view * model * vec4(0, 0, n, 1)).xyz;
}