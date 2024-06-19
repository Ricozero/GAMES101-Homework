#include <iostream>

#include <imgui.h>
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "application.h"

// #define USE_2D

Application::Application(Config config, Viewer *viewer): config(config), viewer(viewer)
{
    rows = 20;
    cols = 20;
    memset(durations, 0, sizeof(durations));
    shader_index_euler = 4;
    shader_index_verlet = 4;
    first_drag = true;
    yaw = 0;
    pitch = 0;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            view[i][j] = i == j;
    scale = 1;
    gpu_simulation = false;
    shader_compute_spring = nullptr;
    shader_compute_mass = nullptr;
}

void Application::init()
{
    create_scene();
    albedo = load_texture("../../src/textures/albedo.png");
    normal = load_texture("../../src/textures/normal.png");
    metallic = load_texture("../../src/textures/metallic.png");
    roughness = load_texture("../../src/textures/roughness.png");
    ao = load_texture("../../src/textures/ao.png");
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
    net_euler = new Net(Vector3D(-200, -200, -200), Vector3D(200, 200, -100), rows, cols, config.mass, config.k1, config.k2, config.k3, {{rows, 0}, {rows, cols}});
    net_verlet = new Net(Vector3D(-200, -200, 100), Vector3D(200, 200, 200), rows, cols, config.mass, config.k1, config.k2, config.k3, {{rows, 0}, {rows, cols}});
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
    // Rendering
    shader_euler = new Shader("../../src/shaders/common.vs", ("../../src/shaders/" + string(SHADER_NAMES[shader_index_euler]) + ".fs").c_str());
    shader_verlet = new Shader("../../src/shaders/common.vs", ("../../src/shaders/" + string(SHADER_NAMES[shader_index_verlet]) + ".fs").c_str());

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

    shader_euler->Use();
    shader_euler->SetMatrix4f("view", (float*)view);
    shader_euler->SetFloat("scale", scale);
    shader_verlet->Use();
    shader_verlet->SetMatrix4f("view", (float*)view);
    shader_verlet->SetFloat("scale", scale);

    if (is_texture_shader(shader_index_euler))
    {
        shader_euler->Use();
        shader_euler->SetInt("albedoMap", 0);
        shader_euler->SetInt("normalMap", 1);
        shader_euler->SetInt("metallicMap", 2);
        shader_euler->SetInt("roughnessMap", 3);
        shader_euler->SetInt("aoMap", 4);
    }
    if (is_texture_shader(shader_index_verlet))
    {
        shader_verlet->Use();
        shader_verlet->SetInt("albedoMap", 0);
        shader_verlet->SetInt("normalMap", 1);
        shader_verlet->SetInt("metallicMap", 2);
        shader_verlet->SetInt("roughnessMap", 3);
        shader_verlet->SetInt("aoMap", 4);
    }

    // GPU computing
    if (gpu_simulation)
    {
        // int max_cwg_count[3];
        // int max_cwg_size[3];
        // int max_cwg_invocations;
        // for (int i = 0; i < 3; ++i)
        // {
        //     glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, &max_cwg_count[i]);
        //     glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &max_cwg_size[i]);
        // }
        // glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_cwg_invocations);
        // printf("Max work groups: (%d, %d, %d)\n", max_cwg_count[0], max_cwg_count[1], max_cwg_count[2]);
        // printf("Max work group size: (%d, %d, %d)\n", max_cwg_size[0], max_cwg_size[1], max_cwg_size[2]);
        // printf("Max invocation number in a single local work group: %d\n", max_cwg_invocations);
        // printf("Work groups: (%d, %d, %d)\n", compute_work_group[0], compute_work_group[1], compute_work_group[2]);

        shader_euler->Use();
        shader_euler->SetInt("gpuSimulation", 1);
        shader_verlet->Use();
        shader_verlet->SetInt("gpuSimulation", 1);

        shader_compute_spring = new Shader("../../src/shaders/spring.cs");
        shader_compute_mass = new Shader("../../src/shaders/mass.cs");

        // Shared SSBO
        glGenBuffers(1, &ssbo);
        struct Vertex
        {
            alignas(16) float position[3];
            alignas(16) float normal[3];
            Vertex& operator=(const Mass& mass)
            {
                position[0] = (float)mass.position.x;
                position[1] = (float)mass.position.y;
                position[2] = (float)mass.position.z;
                normal[0] = (float)mass.normal.x;
                normal[1] = (float)mass.normal.y;
                normal[2] = (float)mass.normal.z;
                return *this;
            }
        };
        size_t size = net_euler->masses.size() + net_verlet->masses.size();
        Vertex* vertices = new Vertex[size];
        for (int i = 0; i < net_euler->masses.size(); ++i) vertices[i] = *net_euler->masses[i];
        for (int i = 0; i < net_verlet->masses.size(); ++i) vertices[i + net_euler->masses.size()] = *net_verlet->masses[i];
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(Vertex), vertices, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        delete[] vertices;

        // Compute shader only SSBO (Mass)
        glGenBuffers(1, &ssbo_mass);
        struct Mass_
        {
            float velocity[3];
            float mass;
            float forces[3];
            int pinned;
            Mass_& operator=(const Mass& mass)
            {
                memset(velocity, 0, sizeof(velocity));
                this->mass = mass.mass;
                memset(forces, 0, sizeof(forces));
                pinned = mass.pinned;
                return *this;
            }
        };
        Mass_* masses = new Mass_[net_euler->masses.size()];
        for (int i = 0; i < net_euler->masses.size(); ++i) masses[i] = *net_euler->masses[i];
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_mass);
        glBufferData(GL_SHADER_STORAGE_BUFFER, net_euler->masses.size() * sizeof(Mass_), masses, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_mass);
        delete[] masses;

        // Compute shader only SSBO (Spring)
        glGenBuffers(1, &ssbo_spring);
        struct Spring_
        {
            int m1;
            int m2;
            float k;
            float rest_length;
            Spring_& operator=(const Spring& spring)
            {
                m1 = spring.m1;
                m2 = spring.m2;
                k = spring.k;
                rest_length = (float)spring.rest_length;
                return *this;
            }
        };
        Spring_* springs = new Spring_[net_euler->springs.size()];
        for (int i = 0; i < net_euler->springs.size(); ++i) springs[i] = *net_euler->springs[i];
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_spring);
        glBufferData(GL_SHADER_STORAGE_BUFFER, net_euler->springs.size() * sizeof(Spring_), springs, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_spring);
        delete[] springs;
    }
    else
    {
        shader_euler->Use();
        shader_euler->SetInt("gpuSimulation", 0);
        shader_verlet->Use();
        shader_verlet->SetInt("gpuSimulation", 0);
    }
#endif
}

void Application::destroy_shaders()
{
    delete shader_euler;
    delete shader_verlet;
    if (shader_compute_spring)
    {
        delete shader_compute_spring;
        shader_compute_spring = nullptr;
    }
    if (shader_compute_mass)
    {
        delete shader_compute_mass;
        shader_compute_mass = nullptr;
    }
}

void Application::simulate()
{
    if (gpu_simulation)
    {
        shader_compute_spring->Use();
        glDispatchCompute((int)net_euler->springs.size(), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        shader_compute_mass->Use();
        glDispatchCompute((int)net_euler->masses.size(), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        return;
    }

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

void Application::render_nets()
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
    const int len = 9;
    auto vertices = new float[sz][9];

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
            vertices[i][6] = (float)i;
            vertices[i][7] = net_euler->texture[i].first;
            vertices[i][8] = net_euler->texture[i].second;
        }
        shader_euler->Use();
        if (gpu_simulation) shader_euler->SetInt("indexOffset", 0);
        glBindVertexArray(vao_euler);
        glBufferData(GL_ARRAY_BUFFER, net_euler->masses.size() * len * sizeof(float), vertices, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, len * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, len * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, len * sizeof(float), (void*)(6 * sizeof(float)));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, len * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, metallic);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, roughness);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ao);
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
            vertices[i][6] = (float)i;
            vertices[i][7] = net_verlet->texture[i].first;
            vertices[i][8] = net_verlet->texture[i].second;
        }
        shader_verlet->Use();
        if (gpu_simulation) shader_verlet->SetInt("indexOffset", (int)net_euler->masses.size());
        glBindVertexArray(vao_verlet);
        glBufferData(GL_ARRAY_BUFFER, net_verlet->masses.size() * len * sizeof(float), vertices, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, len * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, len * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, len * sizeof(float), (void*)(6 * sizeof(float)));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, len * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, metallic);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, roughness);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ao);
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

        if (ImGui::Checkbox("GPU Simulation (restart immediately)##gpu", &gpu_simulation))
        {
            destroy_scene();
            destroy_shaders();
            create_scene();
            create_shaders();
            resize(screen_width, screen_height);
        }

        ImGui::SetNextItemWidth(SLIDER_WIDTH / 2);
        ImGui::InputInt2("Rows/Cols (needs restart)", &rows);

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
    render_nets();
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
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            view[i][j] = i == j;
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

unsigned int Application::load_texture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}