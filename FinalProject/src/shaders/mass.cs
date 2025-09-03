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

uniform float delta_t;

const vec3 gravity = vec3(0, -1, 0);
const float damping = 0.05f;

void main()
{
    int i = int(gl_GlobalInvocationID.x);
    if (!bool(masses[i].pinned))
    {
        masses[i].forces += gravity * masses[i].mass;
        masses[i].velocity += masses[i].forces / masses[i].mass * delta_t;
        vertices[i].position += masses[i].velocity * delta_t * (1 - damping);
    }
    masses[i].forces = vec3(0, 0, 0);
}