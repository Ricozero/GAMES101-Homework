#ifndef MASS_H
#define MASS_H

#include "CGL/CGL.h"
#include "CGL/vector2D.h"

using namespace std;
using namespace CGL;

struct Mass
{
    Mass(Vector2D position, float mass, bool pinned): start_position(position), position(position), last_position(position), mass(mass), pinned(pinned) {}

    float mass;
    bool pinned;

    Vector2D start_position;
    Vector2D position;

    // explicit Verlet integration
    Vector2D last_position;

    // explicit Euler integration
    Vector2D velocity;
    Vector2D forces;
};

struct Spring
{
    Spring(Mass *a, Mass *b, float ks): m1(a), m2(b), ks(ks), rest_length((a->position - b->position).norm()) {}

    float ks;
    double rest_length;

    Mass *m1;
    Mass *m2;
};

class Object
{
public:
    vector<Mass *> masses;
    vector<Spring *> springs;

    Object() {}
    virtual ~Object()
    {
        for (auto mass: masses)
            delete mass;
        for (auto spring: springs)
            delete spring;
    }

    virtual void SimulateVerlet(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // Simulate one timestep of the rope using explicit Verlet（solving constraints)
            double distance = (s->m1->position - s->m2->position).norm();
            Vector2D force = -s->ks * (s->m1->position - s->m2->position) / distance * (distance - s->rest_length);
            s->m1->forces += force;
            s->m2->forces += -force;
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // Set the new position of the rope mass, add global Verlet damping
                Vector2D temp_position = m->position;
                m->forces += gravity * m->mass;
                m->position = 2 * m->position - m->last_position + m->forces / m->mass * delta_t * delta_t * (1 - 0.05);
                m->last_position = temp_position;
            }
            m->forces = Vector2D(0, 0);
        }
    }

    virtual void SimulateEuler(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // Use Hooke's law to calculate the force on a node
            double distance = (s->m1->position - s->m2->position).norm();
            Vector2D force = -s->ks * (s->m1->position - s->m2->position) / distance * (distance - s->rest_length);
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
                m->position += m->velocity * delta_t * (1 - 0.05);
            }
            // Reset all forces on each mass
            m->forces = Vector2D(0, 0);
        }
    }
};

class Rope : public Object
{
public:
    Rope(Vector2D start, Vector2D end, int num_nodes, float node_mass, float ks, vector<int> pinned_nodes)
    {
        // Create a rope starting at `start`, ending at `end`, and containing `num_nodes` nodes
        masses.push_back(new Mass(start, node_mass, false));
        Vector2D cur_position = start, step = (end - start) / (num_nodes - 1);
        for (int i = 1; i < num_nodes; ++i)
        {
            cur_position += step;
            masses.push_back(new Mass(cur_position, node_mass, false));
            springs.push_back(new Spring(masses[i - 1], masses[i], ks));
        }

        for (auto &i : pinned_nodes)
        {
            masses[i]->pinned = true;
        }
    }
};

class Net : public Object
{
public:
    Net(Vector2D min_point, Vector2D max_point, int num_rows, int num_cols, float node_mass, float ks, vector<pair<int, int>> pinned_nodes)
    {
        double grid_x = (max_point.x - min_point.x) / num_cols;
        double grid_y = (max_point.y - min_point.y) / num_rows;
        for (int i = 0; i <= num_rows; ++i)
            for (int j = 0; j <= num_cols; ++j)
                masses.push_back(new Mass(min_point + Vector2D(j * grid_x, i * grid_y), node_mass, false));
        for (auto x : pinned_nodes)
            masses[x.first * (num_cols + 1) + x.second]->pinned = true;
        // Horizontal
        for (int i = 0; i <= num_rows; ++i)
            for (int j = 0; j < num_cols; ++j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[i * (num_cols + 1) + j + 1], ks));
        // Vertical
        for (int i = 0; i < num_rows; ++i)
            for (int j = 0; j <= num_cols; ++j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[(i + 1) * (num_cols + 1) + j], ks));
        // Diagonal
        for (int i = 0; i < num_rows; ++i)
            for (int j = num_cols; j > 0; --j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[(i + 1) * (num_cols + 1) + j - 1], 0.1f));
        // Antidiagonal
        for (int i = 0; i < num_rows; ++i)
            for (int j = 0; j < num_cols; ++j)
                springs.push_back(new Spring(masses[i * (num_cols + 1) + j], masses[(i + 1) * (num_cols + 1) + j + 1], 0.1f));
    }
};

#endif /* MASS_H */
