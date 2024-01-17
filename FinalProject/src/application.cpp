#include <iostream>

#include <imgui.h>
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#include "application.h"

// #define USE_2D

Application::Application(Config config, Viewer *viewer): config(config), viewer(viewer)
{
    memset(durations, 0, sizeof(durations));
    shader_index_euler = 4;
    shader_index_verlet = 4;
    first_drag = true;
    yaw = 0;
    pitch = 0;
    scale = 1;
}

void Application::init()
{
    create_scene();
    create_shaders();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(viewer->get_window(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
}

Application::~Application()
{
    destroy_scene();
    destroy_shaders();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::create_scene()
{
    int num_rows = 20, num_cols = 20;
    net_euler = new Net(Vector3D(-200, -200, -200), Vector3D(200, 200, -100), num_rows, num_cols, config.mass, config.k1, config.k2, config.k3, {{num_rows, 0}, {num_rows, num_cols}});
    net_verlet = new Net(Vector3D(-200, -200, 100), Vector3D(200, 200, 200), num_rows, num_cols, config.mass, config.k1, config.k2, config.k3, {{num_rows, 0}, {num_rows, num_cols}});
}

void Application::destroy_scene()
{
    delete net_euler;
    delete net_verlet;
}

void Application::create_shaders()
{
#ifdef USE_2D
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glPointSize(8);
    glLineWidth(4);
    glColor3f(1.0, 1.0, 1.0);
#else
    // Initialize shaders
    shader_euler = new Shader("../../src/shaders/common.vs", ("../../src/shaders/" + string(SHADER_NAMES[shader_index_euler]) + ".fs").c_str());
    shader_verlet = new Shader("../../src/shaders/common.vs", ("../../src/shaders/" + string(SHADER_NAMES[shader_index_verlet]) + ".fs").c_str());

    // Initialize buffers
    glGenVertexArrays(1, &vao_euler);
    glGenVertexArrays(1, &vao_verlet);
    glGenBuffers(1, &vbo_euler);
    glGenBuffers(1, &vbo_verlet);
    glGenBuffers(1, &ebo_euler);
    glGenBuffers(1, &ebo_verlet);

    glBindVertexArray(vao_euler);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_euler);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_euler);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, net_euler->mesh.size() * sizeof(int), net_euler->mesh.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao_verlet);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_verlet);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_verlet);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, net_verlet->mesh.size() * sizeof(int), net_verlet->mesh.data(), GL_STATIC_DRAW);

    // Initialize uniforms
    float view[4][4]{0};
    for (int i = 0; i < 4; ++i)
        view[i][i] = 1;
    shader_euler->Use();
    shader_euler->SetMatrix4f("view", (float*)view);
    shader_euler->SetFloat("scale", 1);
    shader_verlet->Use();
    shader_verlet->SetMatrix4f("view", (float*)view);
    shader_verlet->SetFloat("scale", 1);
#endif
}

void Application::destroy_shaders()
{
    delete shader_euler;
    delete shader_verlet;
}

void Application::simulate()
{
    if (config.realtime)
    {
        static auto t_old = chrono::high_resolution_clock::now();
        auto t_now = chrono::high_resolution_clock::now();
        float t_elapsed = chrono::duration<float>(t_now - t_old).count();
        t_old = t_now;
        if (config.simulate_euler)
            net_euler->SimulateEuler(t_elapsed, config.gravity, config.damping);
        if (config.simulate_verlet)
            net_verlet->SimulateVerlet(t_elapsed, config.gravity, config.damping);
    }
    else
    {
        for (int i = 0; i < config.steps_per_frame; i++)
        {
            if (config.simulate_euler)
                net_euler->SimulateEuler(1 / (float)config.steps_per_frame, config.gravity, config.damping);
            if (config.simulate_verlet)
                net_verlet->SimulateVerlet(1 / (float)config.steps_per_frame, config.gravity, config.damping);
        }
    }
}

void Application::render_ropes()
{
#ifdef USE_2D
    const Net *object;
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            glColor3f(0.0, 0.0, 1.0);
            object = net_euler;
        }
        else
        {
            glColor3f(0.0, 1.0, 0.0);
            object = net_verlet;
        }

        glBegin(GL_POINTS);
        for (auto &m : object->masses)
        {
            Vector3D p = m->position;
            glVertex2d(p.x, p.y);
        }
        glEnd();

        glBegin(GL_LINES);
        for (auto &s : object->springs)
        {
            Vector3D p1 = s->m1->position;
            Vector3D p2 = s->m2->position;
            glVertex2d(p1.x, p1.y);
            glVertex2d(p2.x, p2.y);
        }
        glEnd();

        glFlush();
    }
#else
    glEnable(GL_DEPTH_TEST);
    if (config.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    size_t sz = max(net_euler->masses.size(), net_verlet->masses.size());
    auto vertices = new float[sz][6];

    if (config.render_euler)
    {
        for (int i = 0; i < net_euler->masses.size(); ++i)
        {
            vertices[i][0] = (float)net_euler->masses[i]->position.x;
            vertices[i][1] = (float)net_euler->masses[i]->position.y;
            vertices[i][2] = (float)net_euler->masses[i]->position.z;
            vertices[i][3] = (float)net_euler->masses[i]->normal.x;
            vertices[i][4] = (float)net_euler->masses[i]->normal.y;
            vertices[i][5] = (float)net_euler->masses[i]->normal.z;
        }
        shader_euler->Use();
        glBindVertexArray(vao_euler);
        glBufferData(GL_ARRAY_BUFFER, net_euler->masses.size() * 6 * sizeof(float), vertices, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawElements(GL_TRIANGLES, (GLsizei)net_euler->mesh.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    if (config.render_verlet)
    {
        for (int i = 0; i < net_verlet->masses.size(); ++i)
        {
            vertices[i][0] = (float)net_verlet->masses[i]->position.x;
            vertices[i][1] = (float)net_verlet->masses[i]->position.y;
            vertices[i][2] = (float)net_verlet->masses[i]->position.z;
            vertices[i][3] = (float)net_verlet->masses[i]->normal.x;
            vertices[i][4] = (float)net_verlet->masses[i]->normal.y;
            vertices[i][5] = (float)net_verlet->masses[i]->normal.z;
        }
        shader_verlet->Use();
        glBindVertexArray(vao_verlet);
        glBufferData(GL_ARRAY_BUFFER, net_verlet->masses.size() * 6 * sizeof(float), vertices, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawElements(GL_TRIANGLES, (GLsizei)net_verlet->mesh.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glDisable(GL_DEPTH_TEST);
    if (config.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    delete[] vertices;
#endif
}

void Application::render_config_window()
{
    static const int SLIDER_WIDTH = 150;
    static const int COMBO_WIDTH = 100;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowSize({0, 0});
    {
        ImGui::Begin("Config");

        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        ImGui::SliderFloat("mass (needs restart)", &config.mass, 0, 10);
        ImGui::SameLine(); if (ImGui::Button("Reset##mass")) config.mass = DEFAULT_MASS;

        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        ImGui::SliderFloat("k1 (needs restart)", &config.k1, 0, 1000);
        ImGui::SameLine(); if (ImGui::Button("Reset##k1")) config.k1 = DEFAULT_K1;
        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        ImGui::SliderFloat("k2 (needs restart)", &config.k2, 0, 10);
        ImGui::SameLine(); if (ImGui::Button("Reset##k2")) config.k2 = DEFAULT_K2;
        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        ImGui::SliderFloat("k3 (needs restart)", &config.k3, 0, 10);
        ImGui::SameLine(); if (ImGui::Button("Reset##k3")) config.k3 = DEFAULT_K3;

        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        ImGui::SliderFloat("damping", &config.damping, 0, 1);
        ImGui::SameLine(); if (ImGui::Button("Reset##damping")) config.damping = DEFAULT_DAMPING;

        static float gravity = 1;
        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        ImGui::SliderFloat("gravity", &gravity, -10, 10);
        config.gravity = gravity * DEFAULT_GRAVITY;
        ImGui::SameLine();
        if (ImGui::Button("Reset##gravity"))
        {
            gravity = 1;
            config.gravity = DEFAULT_GRAVITY;
        }

        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        ImGui::SliderInt("steps_per_frame", &config.steps_per_frame, 0, 1000);
        ImGui::SameLine(); if (ImGui::Button("Reset##steps_per_frame")) config.steps_per_frame = DEFAULT_STEPS_PER_FRAME;

        if (ImGui::Button(("Switch to " + string(config.realtime ? "simulation" : "realtime") + " mode").c_str()))
            config.realtime = !config.realtime;

        ImGui::SameLine();
        if (ImGui::Button("Restart simulation"))
        {
            destroy_scene();
            create_scene();
        }

#ifndef USE_2D
        ImGui::SeparatorText("3D only options");
        auto concat = [](const char* strings[], int size) {
            static vector<char> tmp;
            for (int i = 0; i < size; ++i)
            {
                int len = (int)strlen(strings[i]);
                for (int j = 0; j < len; ++j)
                    tmp.push_back(strings[i][j]);
                tmp.push_back('\0');
            }
            tmp.push_back('\0');
            return tmp.data();
        };
        static const char* SHADER_NAMES_COMBO = concat(SHADER_NAMES, sizeof(SHADER_NAMES) / sizeof(char*));

        ImGui::Text("Euler:");
        ImGui::SameLine(); ImGui::Checkbox("Render##euler", &config.render_euler);
        ImGui::SameLine(); ImGui::Checkbox("Simulate##euler", &config.simulate_euler);
        ImGui::SetNextItemWidth(COMBO_WIDTH);
        ImGui::SameLine(); ImGui::Combo("##euler", &shader_index_euler, SHADER_NAMES_COMBO);

        ImGui::Text("Verlet:");
        ImGui::SameLine(); ImGui::Checkbox("Render##verlet", &config.render_verlet);
        ImGui::SameLine(); ImGui::Checkbox("Simulate##verlet", &config.simulate_verlet);
        ImGui::SetNextItemWidth(COMBO_WIDTH);
        ImGui::SameLine(); ImGui::Combo("##verlet", &shader_index_verlet, SHADER_NAMES_COMBO);

        if (ImGui::Button(("Switch to " + string(config.wireframe ? "normal" : "wireframe") + " mode").c_str()))
            config.wireframe = !config.wireframe;
        ImGui::SameLine();
        if (ImGui::Button("Reload shaders"))
        {
            destroy_shaders();
            create_shaders();
            resize(screen_width, screen_height);
        }
#endif

        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::render()
{
    auto t0 = chrono::high_resolution_clock::now();
    simulate();
    auto t1 = chrono::high_resolution_clock::now();
    render_ropes();
    render_config_window();
    auto t2 = chrono::high_resolution_clock::now();
    durations[0] = chrono::duration<float, milli>(t2 - t0).count();
    durations[1] = chrono::duration<float, milli>(t1 - t0).count();
    durations[2] = chrono::duration<float, milli>(t2 - t1).count();
}

void Application::resize(size_t w, size_t h)
{
    screen_width = w;
    screen_height = h;

#ifdef USE_2D
    float half_width = (float)screen_width / 2;
    float half_height = (float)screen_height / 2;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-half_width, half_width, -half_height, half_height, 1, 0);
#else
    shader_euler->Use();
    shader_euler->SetInt("screenWidth", (int)screen_width);
    shader_euler->SetInt("screenHeight", (int)screen_height);
    shader_verlet->Use();
    shader_verlet->SetInt("screenWidth", (int)screen_width);
    shader_verlet->SetInt("screenHeight", (int)screen_height);
#endif
}

void Application::mouse_button_event(int button, int event)
{
    if (button == GLFW_MOUSE_BUTTON_1 && event == GLFW_RELEASE)
    {
        first_drag = true;
    }
}

void Application::cursor_event(float x, float y, unsigned char keys)
{
#ifndef USE_2D
    if (glfwGetMouseButton(viewer->get_window(), GLFW_MOUSE_BUTTON_1) != GLFW_PRESS)
        return;
    if (ImGui::GetIO().WantCaptureMouse) return;
    static float x_old, y_old;
    if (first_drag)
    {
        x_old = x;
        y_old = y;
        first_drag = false;
        return;
    }
    float x_offset = x - x_old;
    float y_offset = y_old - y;
    x_old = x;
    y_old = y;

    float sensitivity = 0.2f;
    x_offset *= sensitivity;
    y_offset *= sensitivity;
    yaw   += x_offset;
    pitch += y_offset;
    yaw -= ((int)yaw / 360) * 360;
    if (pitch > 89.9f) pitch = 89.9f;
    if (pitch < -89.9f) pitch = -89.9f;

    auto radians = [](float x){ return x * 3.1415926f / 180.0f; };
    float yaw_ = radians(yaw);
    float pitch_ = -radians(pitch);
    float view[4][4]{0};
    for (int i = 0; i < 4; ++i)
        view[i][i] = 1;
    view[0][0] = cos(yaw_);
    view[0][1] = 0;
    view[0][2] = sin(yaw_);
    view[1][0] = sin(yaw_) * sin(pitch_);
    view[1][1] = cos(pitch_);
    view[1][2] = -cos(yaw_) * sin(pitch_);
    view[2][0] = -sin(yaw_);
    view[2][1] = cos(yaw_) * sin(pitch_);
    view[2][2] = cos(yaw_) * cos(pitch_);
    shader_euler->Use();
    shader_euler->SetMatrix4f("view", (float*)view);
    shader_verlet->Use();
    shader_verlet->SetMatrix4f("view", (float*)view);
#endif
}

void Application::scroll_event(float offset_x, float offset_y)
{
#ifndef USE_2D
    float sensitivity = 0.05f;
    scale *= (1 + sensitivity * offset_y);
    shader_euler->Use();
    shader_euler->SetFloat("scale", scale);
    shader_verlet->Use();
    shader_verlet->SetFloat("scale", scale);
#endif
}

string Application::info()
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "%.1f/%.1f/%.1f", durations[0], durations[1], durations[2]);
    return buf;
}