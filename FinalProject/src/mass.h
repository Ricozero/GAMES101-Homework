#ifndef MASS_H
#define MASS_H

#include "CGL/CGL.h"
#include "CGL/vector3D.h"

using namespace std;
using namespace CGL;

struct Mass
{
    Mass(Vector3D position, float mass, bool pinned) : start_position(position), position(position), last_position(position), mass(mass), pinned(pinned) {}

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
    Spring(Mass *m1, Mass *m2, float k) : m1(m1), m2(m2), k(k), rest_length((m1->position - m2->position).norm()) {}

    float k;
    double rest_length;

    Mass *m1;
    Mass *m2;
};

class Net
{
public:
    vector<Mass *> masses;
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
                masses.push_back(new Mass(min_point + Vector3D(j * grid_x, i * grid_y, i * grid_z), node_mass, false));
        for (auto x : pinned_nodes)
            masses[x.first * (num_cols + 1) + x.second]->pinned = true;

        // Horizontal - Structural
        for (int i = 0; i <= num_rows; ++i)
            for (int j = 0; j < num_cols; ++j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[i * (num_cols + 1) + j + 1], k1));
        // Vertical - Structural
        for (int i = 0; i < num_rows; ++i)
            for (int j = 0; j <= num_cols; ++j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[(i + 1) * (num_cols + 1) + j], k1));
        // Diagonal - Shear
        for (int i = 0; i < num_rows; ++i)
            for (int j = num_cols; j > 0; --j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[(i + 1) * (num_cols + 1) + j - 1], k2));
        // Antidiagonal - Shear
        for (int i = 0; i < num_rows; ++i)
            for (int j = 0; j < num_cols; ++j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[(i + 1) * (num_cols + 1) + j + 1], k2));
        // Horizontal - Flexion
        for (int i = 0; i <= num_rows; ++i)
            for (int j = 0; j < num_cols - 1; ++j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[i * (num_cols + 1) + j + 2], k3));
        // Vertical - Flexion
        for (int i = 0; i < num_rows - 1; ++i)
            for (int j = 0; j <= num_cols; ++j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[(i + 2) * (num_cols + 1) + j], k3));

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
        for (auto m : masses)
            delete m;
        for (auto s : springs)
            delete s;
    }

    void CalculateNormal()
    {
        vector<int> share_num(masses.size(), 0);
        for (int i = 0; i < mesh.size(); i+=3)
        {
            Vector3D ab = masses[mesh[i + 1]]->position - masses[mesh[i]]->position;
            Vector3D ac = masses[mesh[i + 2]]->position - masses[mesh[i]]->position;
            Vector3D normal = cross(ab, ac);
            normal.normalize();
            masses[mesh[i]]->normal += normal;
            masses[mesh[i + 1]]->normal += normal;
            masses[mesh[i + 2]]->normal += normal;
            share_num[mesh[i]]++;
            share_num[mesh[i + 1]]++;
            share_num[mesh[i + 2]]++;
        }
        for (int i = 0; i < masses.size(); ++i)
            masses[i]->normal = masses[i]->normal / share_num[i];
    }

    void SimulateVerlet(float delta_t, Vector3D gravity, float damping)
    {
        for (auto &s : springs)
        {
            // Simulate one timestep of the rope using explicit Verlet（solving constraints)
            double distance = (s->m1->position - s->m2->position).norm();
            Vector3D force = -s->k * (s->m1->position - s->m2->position) / distance * (distance - s->rest_length);
            s->m1->forces += force;
            s->m2->forces += -force;
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // Set the new position of the rope mass, add global Verlet damping
                Vector3D temp_position = m->position;
                m->forces += gravity * m->mass;
                m->position = 2 * m->position - m->last_position + m->forces / m->mass * delta_t * delta_t * (1 - damping);
                m->last_position = temp_position;
            }
            m->forces = Vector3D(0, 0, 0);
        }

        // CalculateNormal();
    }

    void SimulateEuler(float delta_t, Vector3D gravity, float damping)
    {
        for (auto &s : springs)
        {
            // Use Hooke's law to calculate the force on a node
            double distance = (s->m1->position - s->m2->position).norm();
            Vector3D force = -s->k * (s->m1->position - s->m2->position) / distance * (distance - s->rest_length);
            s->m1->forces += force;
            s->m2->forces += -force;
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // Add the force due to gravity, then compute the new velocity and position, add global damping
                m->forces += gravity * m->mass;
                m->velocity += m->forces / m->mass * delta_t;
                m->position += m->velocity * delta_t * (1 - damping);
            }
            // Reset all forces on each mass
            m->forces = Vector3D(0, 0, 0);
        }

        // CalculateNormal();
    }
};

#endif /* MASS_H */
