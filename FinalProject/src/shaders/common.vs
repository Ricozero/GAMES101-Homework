#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
out vec3 viewPos;
out vec3 vertexNormal;

uniform int screenWidth;
uniform int screenHeight;
uniform mat4 view;
uniform float scale;

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

void main()
{
    vec4 view_pos = view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    gl_Position = orth * view_pos;
    viewPos = view_pos.xyz;
    vertexNormal = aNormal;
}