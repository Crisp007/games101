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
        for (int i = 0; i < num_nodes; i++) {
            double x = start.x + (end.x - start.x) / (num_nodes-1)*i;
            double y = start.y + (end.y - start.y) / (num_nodes-1)*i;
            Vector2D position(x, y);
            Mass *mass = new Mass(position, node_mass, false);  
            masses.emplace_back(mass);

            if (i != 0) {
                Spring *spring = new Spring(masses[i-1], mass, k);
                springs.emplace_back(spring);
            }
        }

//     Comment-in this part when you implement the constructor
       for (auto &i : pinned_nodes) {
           masses[i]->pinned = true;
       }

    }

    void Rope::simulateEuler(float delta_t, Vector2D gravity)
    {
        for (auto &s : springs)
        {
            // TODO (Part 2): Use Hooke's law to calculate the force on a node
            
            Mass *m1 = s->m1;
            Mass *m2 = s->m2;
            double cur_length = (m1->position - m2->position).norm();

            Vector2D f_1to2 = -1.0f * s->k * (m1->position - m2->position) / cur_length * (cur_length - s->rest_length);
            m1->forces += f_1to2; // m1 to m2
            m2->forces += -1.0f * f_1to2; // m2 to m1
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // TODO (Part 2): Add the force due to gravity, then compute the new velocity and position
                double damping = 0.003;
                Vector2D f_damping = -1.0f * damping * m->velocity;
                m->forces += (gravity * m->mass + f_damping);
                Vector2D accel = m->forces / m->mass;
                Vector2D next_v = m->velocity + accel * delta_t;
                // explicit method
                // m->position += (m->velocity * delta_t);


                // semi-implicit method
                m->position += (next_v * delta_t);


                m->velocity = next_v;

                

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
            Mass *m1 = s->m1;
            Mass *m2 = s->m2;
            double cur_length = (m1->position - m2->position).norm();
            Vector2D f_1to2 = -1.0f * s->k * (m1->position - m2->position) / cur_length * (cur_length - s->rest_length);
            m1->forces += f_1to2; // m1 to m2
            m2->forces += -1.0f * f_1to2; // m2 to m1
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // TODO (Part 3.1): Set the new position of the rope mass
                m->forces += gravity * m->mass;
                Vector2D accel = m->forces / m->mass;

                Vector2D cur_position = m->position;

                double damping = 0.00005;

                m->position = cur_position + (1 - damping) * (cur_position - m->last_position) + accel * delta_t * delta_t;
                // m->position = cur_position + (cur_position - m->last_position) + accel * delta_t * delta_t;

                m->last_position = cur_position;
            }
            m->forces = Vector2D(0, 0);
        }
    }
}
