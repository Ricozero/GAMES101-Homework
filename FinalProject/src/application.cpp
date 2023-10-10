#include <iostream>

#include <imgui.h>
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#include "application.h"

Application::~Application()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete ropeEuler;
    delete ropeVerlet;
}

void Application::init()
{
    // Enable anti-aliasing and circular points
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    glPointSize(8);
    glLineWidth(4);
    glColor3f(1.0, 1.0, 1.0);

    // Create two ropes
    ropeEuler = new Rope(Vector2D(0, 200), Vector2D(-400, 200), 3, config.mass, config.ks, {0});
    ropeVerlet = new Rope(Vector2D(0, 200), Vector2D(-400, 200), 3, config.mass, config.ks, {0});

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(viewer->get_window(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
}

void Application::render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowSize({-1, -1});

    {
        ImGui::Begin("Config");

        ImGui::SliderFloat("mass", &config.mass, 0, 10);
        ImGui::SameLine();
        if (ImGui::Button("Reset##1"))
            config.mass = 1;

        ImGui::SliderFloat("ks", &config.ks, 0, 1000);
        ImGui::SameLine();
        if (ImGui::Button("Reset##2"))
            config.ks = 100;

        static float gravity = 1;
        ImGui::SliderFloat("gravity", &gravity, -10, 10);
        config.gravity = gravity * Vector2D(0, -1);
        ImGui::SameLine();
        if (ImGui::Button("Reset##3"))
        {
            gravity = 1;
            config.gravity = Vector2D(0, -1);
        }

        static int steps_per_frame = 64;
        ImGui::SliderInt("steps_per_frame", &steps_per_frame, 0, 1000);
        config.steps_per_frame = (float)steps_per_frame;
        ImGui::SameLine();
        if (ImGui::Button("Reset##4"))
        {
            steps_per_frame = 64;
            config.steps_per_frame = (float)64;
        }

        ImGui::End();
    }

    // Simulation loops
    for (int i = 0; i < config.steps_per_frame; i++)
    {
        ropeEuler->simulateEuler(1 / config.steps_per_frame, config.gravity);
        ropeVerlet->simulateVerlet(1 / config.steps_per_frame, config.gravity);
    }

    // Rendering ropes
    Rope *rope;
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            glColor3f(0.0, 0.0, 1.0);
            rope = ropeEuler;
        }
        else
        {
            glColor3f(0.0, 1.0, 0.0);
            rope = ropeVerlet;
        }

        glBegin(GL_POINTS);
        for (auto &m : rope->masses)
        {
            Vector2D p = m->position;
            glVertex2d(p.x, p.y);
        }
        glEnd();

        glBegin(GL_LINES);
        for (auto &s : rope->springs)
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
