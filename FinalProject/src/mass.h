#ifndef MASS_H
#define MASS_H

#include "CGL/CGL.h"
#include "CGL/vector3D.h"

using namespace std;
using namespace CGL;

struct Vertex
{
    Vertex(Vector3D position, float mass, bool pinned) : start_position(position), position(position), last_position(position), mass(mass), pinned(pinned) {}

    float mass;
    bool pinned;

    Vector3D start_position;
    Vector3D position;

    // explicit Verlet integration
    Vector3D last_position;

    // explicit Euler integration
    Vector3D velocity;
    Vector3D forces;

    // for rendering
    Vector3D normal;
};

struct Spring
{
    Spring(Vertex *a, Vertex *b, float k) : v1(a), v2(b), k(k), rest_length((a->position - b->position).norm()) {}

    float k;
    double rest_length;

    Vertex *v1;
    Vertex *v2;
};

class Net
{
public:
    vector<Vertex *> vertices;
    vector<Spring *> springs;
    vector<unsigned int> mesh;

    Net(Vector3D min_point, Vector3D max_point, int num_rows, int num_cols, float node_mass, float k1, float k2, float k3, vector<pair<int, int>> pinned_nodes)
    {
        // Create masses
        double grid_x = (max_point.x - min_point.x) / num_cols;
        double grid_y = (max_point.y - min_point.y) / num_rows;
        double grid_z = (max_point.z - min_point.z) / num_rows;
        for (int i = 0; i <= num_rows; ++i)
            for (int j = 0; j <= num_cols; ++j)
                vertices.push_back(new Vertex(min_point + Vector3D(j * grid_x, i * grid_y, i * grid_z), node_mass, false));
        for (auto x : pinned_nodes)
            vertices[x.first * (num_cols + 1) + x.second]->pinned = true;

        // Horizontal - Structural
        for (int i = 0; i <= num_rows; ++i)
            for (int j = 0; j < num_cols; ++j)
                springs.push_back(new Spring(vertices[i * (num_cols + 1) + j], vertices[i * (num_cols + 1) + j + 1], k1));
        // Vertical - Structural
        for (int i = 0; i < num_rows; ++i)
            for (int j = 0; j <= num_cols; ++j)
                springs.push_back(new Spring(vertices[i * (num_cols + 1) + j], vertices[(i + 1) * (num_cols + 1) + j], k1));
        // Diagonal - Shear
        for (int i = 0; i < num_rows; ++i)
            for (int j = num_cols; j > 0; --j)
                springs.push_back(new Spring(vertices[i * (num_cols + 1) + j], vertices[(i + 1) * (num_cols + 1) + j - 1], k2));
        // Antidiagonal - Shear
        for (int i = 0; i < num_rows; ++i)
            for (int j = 0; j < num_cols; ++j)
                springs.push_back(new Spring(vertices[i * (num_cols + 1) + j], vertices[(i + 1) * (num_cols + 1) + j + 1], k2));
        // Horizontal - Flexion
        for (int i = 0; i <= num_rows; ++i)
            for (int j = 0; j < num_cols - 1; ++j)
                springs.push_back(new Spring(vertices[i * (num_cols + 1) + j], vertices[i * (num_cols + 1) + j + 2], k3));
        // Vertical - Flexion
        for (int i = 0; i < num_rows - 1; ++i)
            for (int j = 0; j <= num_cols; ++j)
                springs.push_back(new Spring(vertices[i * (num_cols + 1) + j], vertices[(i + 2) * (num_cols + 1) + j], k3));

        // Create mesh
        for (int i = 0; i < num_rows; ++i)
            for (int j = 0; j < num_cols; ++j)
            {
                int index = i * (num_cols + 1) + j;
                mesh.push_back(index);
                mesh.push_back(index + 1);
                mesh.push_back(index + (num_cols + 1) + 1);
                mesh.push_back(index);
                mesh.push_back(index + (num_cols + 1) + 1);
                mesh.push_back(index + (num_cols + 1));
            }
        // CalculateNormal();
    }

    ~Net()
    {
        for (auto vertex : vertices)
            delete vertex;
        for (auto spring : springs)
            delete spring;
    }

    void CalculateNormal()
    {
        vector<int> share_num(vertices.size(), 0);
        for (int i = 0; i < mesh.size(); i+=3)
        {
            Vector3D ab = vertices[mesh[i + 1]]->position - vertices[mesh[i]]->position;
            Vector3D ac = vertices[mesh[i + 2]]->position - vertices[mesh[i]]->position;
            Vector3D normal = cross(ab, ac);
            normal.normalize();
            vertices[mesh[i]]->normal += normal;
            vertices[mesh[i + 1]]->normal += normal;
            vertices[mesh[i + 2]]->normal += normal;
            share_num[mesh[i]]++;
            share_num[mesh[i + 1]]++;
            share_num[mesh[i + 2]]++;
        }
        for (int i = 0; i < vertices.size(); ++i)
            vertices[i]->normal = vertices[i]->normal / share_num[i];
    }

    void SimulateVerlet(float delta_t, Vector3D gravity, float damping)
    {
        for (auto &s : springs)
        {
            // Simulate one timestep of the rope using explicit Verlet（solving constraints)
            double distance = (s->v1->position - s->v2->position).norm();
            Vector3D force = -s->k * (s->v1->position - s->v2->position) / distance * (distance - s->rest_length);
            s->v1->forces += force;
            s->v2->forces += -force;
        }

        for (auto &v : vertices)
        {
            if (!v->pinned)
            {
                // Set the new position of the rope mass, add global Verlet damping
                Vector3D temp_position = v->position;
                v->forces += gravity * v->mass;
                v->position = 2 * v->position - v->last_position + v->forces / v->mass * delta_t * delta_t * (1 - damping);
                v->last_position = temp_position;
            }
            v->forces = Vector3D(0, 0, 0);
        }

        // CalculateNormal();
    }

    void SimulateEuler(float delta_t, Vector3D gravity, float damping)
    {
        for (auto &s : springs)
        {
            // Use Hooke's law to calculate the force on a node
            double distance = (s->v1->position - s->v2->position).norm();
            Vector3D force = -s->k * (s->v1->position - s->v2->position) / distance * (distance - s->rest_length);
            s->v1->forces += force;
            s->v2->forces += -force;
        }

        for (auto &v : vertices)
        {
            if (!v->pinned)
            {
                // Add the force due to gravity, then compute the new velocity and position, add global damping
                v->forces += gravity * v->mass;
                v->velocity += v->forces / v->mass * delta_t;
                v->position += v->velocity * delta_t * (1 - damping);
            }
            // Reset all forces on each mass
            v->forces = Vector3D(0, 0, 0);
        }

        // CalculateNormal();
    }
};

#endif /* MASS_H */
