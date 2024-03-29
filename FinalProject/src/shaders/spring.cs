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

struct Mass
{
    vec3 velocity;
    float mass;
    vec3 forces;
    int pinned;
};
layout (std430, binding = 1) buffer net_mass
{
    Mass masses[];
};

struct Spring
{
    int k;
    int rest_length;
    int m1;
    int m2;
};
layout (std430, binding = 2) buffer net_spring
{
    Spring springs[];
};

void main()
{
    Spring s = springs[gl_GlobalInvocationID.x];
    float d = length(vertices[s.m1].position - vertices[s.m2].position);
    vec3 force = -s.k * (vertices[s.m1].position - vertices[s.m2].position) / d * (d - s.rest_length);
    masses[s.m1].forces += force;
    masses[s.m2].forces -= force;
}