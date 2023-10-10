#ifndef APPLICATION_H
#define APPLICATION_H

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CGL/vector2D.h"
#include "CGL/renderer.h"

#include "mass.h"

struct AppConfig
{
    AppConfig()
    {
        // Rope config variables
        mass = 1;
        ks = 100;

        // Environment variables
        gravity = Vector2D(0, -1);
        steps_per_frame = 64;
    }

    float mass;
    float ks;

    float steps_per_frame;
    Vector2D gravity;
};

class Application : public Renderer
{
public:
    Application(AppConfig config, Viewer *viewer): config(config), viewer(viewer) {}
    ~Application();

    void init();
    void render();
    void resize(size_t w, size_t h);

    string name() { return "Rope Simulator"; }
    string info()
    {
        const static string prompt = "FPS: ";
        static auto t_old = std::chrono::high_resolution_clock::now();
        auto t_now = std::chrono::high_resolution_clock::now();
        double t_elapsed = std::chrono::duration<double, std::milli>(t_now - t_old).count();
        t_old = t_now;
        int fps = int(1000 / t_elapsed);
        return prompt + to_string(fps);
    }

private:
    const Viewer *viewer;
    AppConfig config;

    Rope *ropeEuler;
    Rope *ropeVerlet;

    size_t screen_width;
    size_t screen_height;
};

#endif // APPLICATION_H
