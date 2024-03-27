#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
struct Vertex
{
    vec3 position;
    vec3 normal;
};
layout (std430, binding = 0) buffer net
{
    Vertex vertices[];
};

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
}