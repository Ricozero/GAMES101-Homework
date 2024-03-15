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
#include "shader.h"

static const float DEFAULT_MASS = 1;
static const float DEFAULT_K1 = 100;
static const float DEFAULT_K2 = 0.1f;
static const float DEFAULT_K3 = 0.1f;
static const float DEFAULT_DAMPING = 0.05f;
static const Vector3D DEFAULT_GRAVITY = Vector3D(0, -1, 0);
static const int DEFAULT_STEPS_PER_FRAME = 64;
static const char* SHADER_NAMES[] = { "blue", "green", "normal", "phong", "phong_tex", "pbr", "pbr_tex" };

struct Config
{
    Config()
    {
        mass = DEFAULT_MASS;
        k1 = DEFAULT_K1;
        k2 = DEFAULT_K2;
        k3 = DEFAULT_K3;
        damping = DEFAULT_DAMPING;
        gravity = DEFAULT_GRAVITY;
        steps_per_frame = DEFAULT_STEPS_PER_FRAME;
        realtime = false;
        wireframe = false;
        render_euler = true;
        simulate_euler = true;
        render_verlet = true;
        simulate_verlet = true;
    }

    // Rope config variables
    float mass;
    float k1;
    float k2;
    float k3;
    float damping;

    // Environment variables
    Vector3D gravity;
    int steps_per_frame;
    bool realtime;

    // Rendering variables
    bool wireframe;
    bool render_euler;
    bool simulate_euler;
    bool render_verlet;
    bool simulate_verlet;
};

class Application : public Renderer
{
public:
    Application(Config config, Viewer *viewer);
    ~Application();

    void init();
    void create_scene();
    void destroy_scene();
    void create_shaders();
    void destroy_shaders();
    void simulate();
    void render_nets();
    void render_config_window();
    void render();
    void resize(size_t w, size_t h);
    void mouse_button_event(int button, int event);
    void cursor_event(float x, float y, unsigned char keys);
    void scroll_event(float offset_x, float offset_y);
    unsigned int load_texture(const char* path);
    static bool is_texture_shader(int index) { return index == 4 || index == 6; }

    string name() { return "Rope Simulator"; }
    string info();

private:
    const Viewer *viewer;
    Config config;

    size_t screen_width;
    size_t screen_height;

    int rows;
    int cols;
    Net *net_euler;
    Net *net_verlet;

    float durations[3];

    int shader_index_euler;
    int shader_index_verlet;
    Shader *shader_euler;
    Shader *shader_verlet;
    unsigned int vao_euler;
    unsigned int vao_verlet;
    unsigned int vbo_euler;
    unsigned int vbo_verlet;
    unsigned int ebo_euler;
    unsigned int ebo_verlet;
    unsigned int albedo;
    unsigned int normal;
    unsigned int metallic;
    unsigned int roughness;
    unsigned int ao;
    bool first_drag;
    float yaw;
    float pitch;
    float view[4][4];
    float scale;

    bool gpu_simulation;
    Shader *shader_compute;
    unsigned int ssbo;
    int compute_work_group[3];
};

#endif // APPLICATION_H
