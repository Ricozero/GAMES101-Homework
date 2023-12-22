#version 330 core
in vec3 vertexNormal;
out vec4 fragColor;

// const vec3 ka = vec3(0.005, 0.005, 0.005);
// const vec3 kd = fragColor.xyz;
// const vec3 ks = vec3(0.7937, 0.7937, 0.7937);

// struct light
// {
//     vec3 pos;
//     vec3 intensity;
// };
// const light lights[2] = {
//     light(vec3(20, 20, 20), vec3(500, 500, 500)),
//     light(vec3(-20, 20, 0), vec3(500, 500, 500))
// };
// const light l1 = light(vec3(20, 20, 20), vec3(500, 500, 500));
// const light l2 = light(vec3(-20, 20, 0), vec3(500, 500, 500));

// const vec3 amb_light_intensity = vec3(10, 10, 10);
// const vec3 eye_pos = vec3(0, 0, 10);

// const float p = 150;

// vec3 color = payload.color;
// vec3 point = payload.view_pos;
// vec3 normal = payload.normal;

// vec3 result_color = vec3(0, 0, 0);
// for (auto& light : lights)
// {
//     // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
//     // components are. Then, accumulate that result on the *result_color* object.
//     Vector3f La = ka.cwiseProduct(amb_light_intensity);
//     float r = (light.position - point).norm();
//     Vector3f Ld = kd.cwiseProduct(light.intensity) / (r * r) * std::max(0.0f, normal.dot((light.position - point).normalized()));
//     Vector3f Ls = ks.cwiseProduct(light.intensity) / (r * r) * std::powf(std::max(0.0f, normal.dot(((light.position - point + eye_pos - point) / 2).normalized())), p);
//     result_color += La + Ld + Ls;
// }

void main()
{
    // fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    fragColor = vec4(vertexNormal.x, vertexNormal.y, vertexNormal.z, 1.0f);
}