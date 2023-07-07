#include <iostream>
#include <vector>

#include "CGL/vector2D.h"

#include "mass.h"
#include "rope.h"
#include "spring.h"

namespace CGL {

    Rope::Rope(Vector2D start, Vector2D end, int num_nodes, float node_mass, float k, vector<int> pinned_nodes)
    {
        // TODO (Part 1): Create a rope starting at `start`, ending at `end`, and containing `num_nodes` nodes.
        masses.push_back(new Mass(start, node_mass, false));
        Vector2D cur_position = start, step = (end - start) / (num_nodes - 1);
        for (int i = 1; i < num_nodes; ++i) {
            cur_position += step;
            masses.push_back(new Mass(cur_position, node_mass, false));
            springs.push_back(new Spring(masses[i - 1], masses[i], k));
        }

       for (auto &i : pinned_nodes) {
           masses[i]->pinned = true;
       }
    }

    void Rope::simulateEuler(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // TODO (Part 2): Use Hooke's law to calculate the force on a node
            double distance = (s->m1->position - s->m2->position).norm();
            Vector2D force = -s->k * (s->m1->position - s->m2->position) / distance * (distance - s->rest_length);
            s->m1->forces += force;
            s->m2->forces += -force;
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // TODO (Part 2): Add the force due to gravity, then compute the new velocity and position
                m->forces += gravity * m->mass;
                m->velocity += m->forces / m->mass * delta_t;
                m->position += m->velocity * delta_t * (1 - 0.05);
                // TODO (Part 2): Add global damping
            }

            // Reset all forces on each mass
            m->forces = Vector2D(0, 0);
        }
    }

    void Rope::simulateVerlet(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // TODO (Part 3): Simulate one timestep of the rope using explicit Verlet （solving constraints)
            double distance = (s->m1->position - s->m2->position).norm();
            Vector2D force = -s->k * (s->m1->position - s->m2->position) / distance * (distance - s->rest_length);
            s->m1->forces += force;
            s->m2->forces += -force;
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                Vector2D temp_position = m->position;
                // TODO (Part 3.1): Set the new position of the rope mass
                m->forces += gravity * m->mass;
                m->position = 2 * m->position - m->last_position + m->forces / m->mass * delta_t * delta_t * (1 - 0.05);
                m->last_position = temp_position;
                // TODO (Part 4): Add global Verlet damping
            }
            m->forces = Vector2D(0, 0);
        }
    }
}
