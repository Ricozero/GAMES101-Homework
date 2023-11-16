#ifndef APPLICATION_H
#define APPLICATION_H

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CGL/vector3D.h"
#include "CGL/renderer.h"

#include "mass.h"

static const float DEFAULT_MASS = 1;
static const float DEFAULT_KS = 100;
static const Vector3D DEFAULT_GRAVITY = Vector3D(0, -1, 0);
static const float DEFAULT_STEPS_PER_FRAME = 64;

struct Config
{
    Config()
    {
        mass = DEFAULT_MASS;
        ks = DEFAULT_KS;
        gravity = DEFAULT_GRAVITY;
        steps_per_frame = DEFAULT_STEPS_PER_FRAME;
        realtime = false;
        wireframe = false;
    }

    // Rope config variables
    float mass;
    float ks;

    // Environment variables
    Vector3D gravity;
    float steps_per_frame;
    bool realtime;

    // Rendering variables
    bool wireframe;
};

class Application : public Renderer
{
public:
    Application(Config config, Viewer *viewer): config(config), viewer(viewer) {}
    ~Application();

    void init();
    void create_scene();
    void destroy_scene();
    void update();
    void render();
    void resize(size_t w, size_t h);

    string name() { return "Rope Simulator"; }
    string info()
    {
        const static string prompt = "FPS: ";
        static auto t_old = chrono::high_resolution_clock::now();
        auto t_now = chrono::high_resolution_clock::now();
        double t_elapsed = chrono::duration<double>(t_now - t_old).count();
        t_old = t_now;
        int fps = int(1 / t_elapsed);
        return prompt + to_string(fps);
    }

private:
    const Viewer *viewer;
    Config config;

    size_t screen_width;
    size_t screen_height;

    Net *net_euler;
    Net *net_verlet;
    unsigned int vao_euler;
    unsigned int vao_verlet;
    unsigned int vbo_euler;
    unsigned int vbo_verlet;
    unsigned int ebo_euler;
    unsigned int ebo_verlet;
    unsigned int shader_program_euler;
    unsigned int shader_program_verlet;
};

#endif // APPLICATION_H
