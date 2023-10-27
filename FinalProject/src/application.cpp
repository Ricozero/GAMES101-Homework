#include <iostream>

#include <imgui.h>
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#include "application.h"

void Application::init()
{
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glPointSize(8);
    glLineWidth(4);
    glColor3f(1.0, 1.0, 1.0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(viewer->get_window(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    create_scene();
}

Application::~Application()
{
    destroy_scene();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::create_scene()
{
    // object_euler = new Rope(Vector2D(0, 200), Vector2D(-400, 200), 5, config.mass, config.ks, {0, 4});
    // object_verlet = new Rope(Vector2D(0, 200), Vector2D(-400, 200), 5, config.mass, config.ks, {0, 4});
    int num_rows = 20, num_cols = 20;
    object_euler = new Net(Vector2D(-200, -200), Vector2D(200, 200), num_rows, num_cols, config.mass, config.ks, {{num_rows, 0}, {num_rows, num_cols}});
    object_verlet = new Net(Vector2D(-200, -200), Vector2D(200, 200), num_rows, num_cols, config.mass, config.ks, {{num_rows, 0}, {num_rows, num_cols}});
}

void Application::destroy_scene()
{
    delete object_euler;
    delete object_verlet;
}

void Application::render()
{
    // Render config window
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowSize({-1, -1});
    {
        ImGui::Begin("Config");

        ImGui::SliderFloat("mass (needs restart)", &config.mass, 0, 10);
        ImGui::SameLine();
        if (ImGui::Button("Reset##mass"))
            config.mass = DEFAULT_MASS;

        ImGui::SliderFloat("ks (needs restart)", &config.ks, 0, 1000);
        ImGui::SameLine();
        if (ImGui::Button("Reset##ks"))
            config.ks = DEFAULT_KS;

        static float gravity = 1;
        ImGui::SliderFloat("gravity", &gravity, -10, 10);
        config.gravity = gravity * DEFAULT_GRAVITY;
        ImGui::SameLine();
        if (ImGui::Button("Reset##gravity"))
        {
            gravity = 1;
            config.gravity = DEFAULT_GRAVITY;
        }

        static int steps_per_frame = (int)DEFAULT_STEPS_PER_FRAME;
        ImGui::SliderInt("steps_per_frame", &steps_per_frame, 0, 1000);
        config.steps_per_frame = (float)steps_per_frame;
        ImGui::SameLine();
        if (ImGui::Button("Reset##steps_per_frame"))
        {
            steps_per_frame = (int)DEFAULT_STEPS_PER_FRAME;
            config.steps_per_frame = DEFAULT_STEPS_PER_FRAME;
        }

        if (ImGui::Button(("Switch to " + string(config.realtime ? "simulation" : "realtime") + " mode").c_str()))
            config.realtime = !config.realtime;

        if (ImGui::Button("Restart simulation"))
        {
            destroy_scene();
            create_scene();
        }

        ImGui::End();
    }

    // Simulation loop
    if (config.realtime)
    {
        static auto t_old = chrono::high_resolution_clock::now();
        auto t_now = chrono::high_resolution_clock::now();
        float t_elapsed = chrono::duration<float>(t_now - t_old).count();
        t_old = t_now;
        object_euler->SimulateEuler(t_elapsed, config.gravity);
        object_verlet->SimulateVerlet(t_elapsed, config.gravity);
    }
    else
    {
        for (int i = 0; i < config.steps_per_frame; i++)
        {
            object_euler->SimulateEuler(1 / config.steps_per_frame, config.gravity);
            object_verlet->SimulateVerlet(1 / config.steps_per_frame, config.gravity);
        }
    }

    // Render ropes
    const Object *object;
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            glColor3f(0.0, 0.0, 1.0);
            object = object_euler;
        }
        else
        {
            glColor3f(0.0, 1.0, 0.0);
            object = object_verlet;
        }

        glBegin(GL_POINTS);
        for (auto &m : object->masses)
        {
            Vector2D p = m->position;
            glVertex2d(p.x, p.y);
        }
        glEnd();

        glBegin(GL_LINES);
        for (auto &s : object->springs)
        {
            Vector2D p1 = s->m1->position;
            Vector2D p2 = s->m2->position;
            glVertex2d(p1.x, p1.y);
            glVertex2d(p2.x, p2.y);
        }
        glEnd();

        glFlush();
    }
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::resize(size_t w, size_t h)
{
    screen_width = w;
    screen_height = h;

    float half_width = (float)screen_width / 2;
    float half_height = (float)screen_height / 2;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-half_width, half_width, -half_height, half_height, 1, 0);
}
