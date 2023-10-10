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
    Spring(Mass *a, Mass *b, float k): m1(a), m2(b), k(k), rest_length((a->position - b->position).norm()) {}

    float k;
    double rest_length;

    Mass *m1;
    Mass *m2;
};

class Rope
{
public:
    Rope(vector<Mass *> &masses, vector<Spring *> &springs): masses(masses), springs(springs) {}
    Rope(Vector2D start, Vector2D end, int num_nodes, float node_mass, float k, vector<int> pinned_nodes)
    {
        // (Part 1): Create a rope starting at `start`, ending at `end`, and containing `num_nodes` nodes
        masses.push_back(new Mass(start, node_mass, false));
        Vector2D cur_position = start, step = (end - start) / (num_nodes - 1);
        for (int i = 1; i < num_nodes; ++i)
        {
            cur_position += step;
            masses.push_back(new Mass(cur_position, node_mass, false));
            springs.push_back(new Spring(masses[i - 1], masses[i], k));
        }

        for (auto &i : pinned_nodes)
        {
            masses[i]->pinned = true;
        }
    }
    ~Rope()
    {
        for (auto mass: masses)
            delete mass;
        for (auto spring: springs)
            delete spring;
    }

    void simulateVerlet(float delta_t, Vector2D gravity)
    {
    for (auto &s : springs)
        {
            // (Part 3): Simulate one timestep of the rope using explicit Verlet （solving constraints)
            double distance = (s->m1->position - s->m2->position).norm();
            Vector2D force = -s->k * (s->m1->position - s->m2->position) / distance * (distance - s->rest_length);
            s->m1->forces += force;
            s->m2->forces += -force;
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // (Part 3.1): Set the new position of the rope mass
                // (Part 4): Add global Verlet damping
                Vector2D temp_position = m->position;
                m->forces += gravity * m->mass;
                m->position = 2 * m->position - m->last_position + m->forces / m->mass * delta_t * delta_t * (1 - 0.05);
                m->last_position = temp_position;
            }
            m->forces = Vector2D(0, 0);
        }
    }
    
    void simulateEuler(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // (Part 2): Use Hooke's law to calculate the force on a node
            double distance = (s->m1->position - s->m2->position).norm();
            Vector2D force = -s->k * (s->m1->position - s->m2->position) / distance * (distance - s->rest_length);
            s->m1->forces += force;
            s->m2->forces += -force;
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // (Part 2): Add the force due to gravity, then compute the new velocity and position
                m->forces += gravity * m->mass;
                m->velocity += m->forces / m->mass * delta_t;
                // (Part 3): Add global damping
                m->position += m->velocity * delta_t * (1 - 0.05);
            }

            // Reset all forces on each mass
            m->forces = Vector2D(0, 0);
        }
    }

    vector<Mass *> masses;
    vector<Spring *> springs;
};


#endif /* MASS_H */
