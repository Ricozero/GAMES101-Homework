#version 330 core
in vec3 vertexPos;
in vec3 vertexNormal;
in vec3 eyePos;
out vec4 fragColor;

const vec3 ka = vec3(0.005, 0.005, 0.005);
const vec3 kd = vec3(0.5f, 1.0f, 0.5f);
const vec3 ks = vec3(0.7937, 0.7937, 0.7937);
const vec3 amb_light_intensity = vec3(10, 10, 10);
const float p = 150;

struct light
{
    vec3 position;
    vec3 intensity;
};
const light lights[2] = light[](
    light(vec3(300, 300, 0), vec3(100000, 100000, 100000)),
    light(vec3(-300, 300, 0), vec3(100000, 100, 100))
);

void main()
{
    vec3 eye_pos = eyePos;
    vec3 view_pos = vertexPos;
    vec3 normal = vertexNormal;
    vec3 color = vec3(0, 0, 0);
    for (int i = 0; i < 2; ++i)
    {
        vec3 La = ka * amb_light_intensity;
        float r = length(lights[i].position - view_pos);
        float ndotl = dot(normal, normalize(lights[i].position - view_pos));
        // vec3 Ld = kd * lights[i].intensity / (r * r) * (sin(ndotl * 3.14159 / 2) + 1);
        vec3 Ld = kd * lights[i].intensity / (r * r) * (ndotl + 1) / 2;
        float ndoth = dot(normal, normalize((lights[i].position - view_pos + eye_pos - view_pos) / 2));
        vec3 Ls = ks * lights[i].intensity / (r * r) * pow(max(0.0f, ndoth), p);
        color += La + Ld + Ls;
    }
    fragColor = vec4(color.x, color.y, color.z, 1.0f);
}