#version 330 core
in vec3 vertexPos;
in vec3 vertexNormal;
out vec4 fragColor;

void main()
{
    const vec3 ka = vec3(0.005, 0.005, 0.005);
    const vec3 kd = vec3(0.0f, 1.0f, 0.0f);
    const vec3 ks = vec3(0.7937, 0.7937, 0.7937);

    struct light
    {
        vec3 position;
        vec3 intensity;
    };
    const light lights[2] ={
        light(vec3(20, 20, 20), vec3(500, 500, 500)),
        light(vec3(-20, 20, 0), vec3(500, 500, 500))
    };

    const vec3 amb_light_intensity = vec3(10, 10, 10);
    const vec3 eye_pos = vec3(0, 0, 500);
    vec3 view_pos = vertexPos;
    vec3 normal = vertexNormal;

    const float p = 150;

    vec3 color = vec3(0, 0, 0);
    int i = 0;
    for (i = 0; i < 2; ++i)
    {
        vec3 La = ka * amb_light_intensity;
        float r = length(lights[i].position - view_pos);
        vec3 Ld = kd * lights[i].intensity / (r * r) * max(0.0f, dot(normal, normalize(lights[i].position - view_pos)));
        vec3 Ls = ks * lights[i].intensity / (r * r) * pow(max(0.0f, dot(normal, normalize((lights[i].position - view_pos + eye_pos - view_pos) / 2))), p);
        color += La + Ld + Ls;
    }
    // fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    // fragColor = vec4(vertexNormal.x, vertexNormal.y, vertexNormal.z, 1.0f);
    fragColor = vec4(color.x, color.y, color.z, 1.0f);
}