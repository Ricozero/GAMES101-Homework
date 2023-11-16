#include <iostream>

#include <imgui.h>
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#include "application.h"

#define USE_2D

void Application::init()
{
    create_scene();

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
    glGenVertexArrays(1, &vao_euler);
    glGenVertexArrays(1, &vao_verlet);
    glGenBuffers(1, &vbo_euler);
    glGenBuffers(1, &vbo_verlet);
    glGenBuffers(1, &ebo_euler);
    glGenBuffers(1, &ebo_verlet);

    int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const char *vertex_shader_source = R"delimiter(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        void main()
        {
            gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        }
    )delimiter";
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    int fragment_shader_euler = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragment_shader_source_euler = R"delimiter(
        #version 330 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
        }
    )delimiter";
    glShaderSource(fragment_shader_euler, 1, &fragment_shader_source_euler, NULL);
    glCompileShader(fragment_shader_euler);
    shader_program_euler = glCreateProgram();
    glAttachShader(shader_program_euler, vertex_shader);
    glAttachShader(shader_program_euler, fragment_shader_euler);
    glLinkProgram(shader_program_euler);
    glDeleteShader(fragment_shader_euler);

    int fragment_shader_verlet = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragment_shader_source_verlet = R"delimiter(
        #version 330 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }
    )delimiter";
    glShaderSource(fragment_shader_verlet, 1, &fragment_shader_source_verlet, NULL);
    glCompileShader(fragment_shader_verlet);
    shader_program_verlet = glCreateProgram();
    glAttachShader(shader_program_verlet, vertex_shader);
    glAttachShader(shader_program_verlet, fragment_shader_verlet);
    glLinkProgram(shader_program_verlet);
    glDeleteShader(fragment_shader_verlet);

    glDeleteShader(vertex_shader);

    glBindVertexArray(vao_euler);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_euler);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_euler);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, net_euler->mesh.size() * sizeof(int), net_euler->mesh.data(), GL_STATIC_DRAW);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(0);

    glBindVertexArray(vao_verlet);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_verlet);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_verlet);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, net_verlet->mesh.size() * sizeof(int), net_verlet->mesh.data(), GL_STATIC_DRAW);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(0);
#endif

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
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::create_scene()
{
    int num_rows = 20, num_cols = 20;
#ifdef USE_2D
    net_euler = new Net(Vector3D(-200, -200, -1), Vector3D(200, 200, -1), num_rows, num_cols, config.mass, config.ks, {{num_rows, 0}, {num_rows, num_cols}});
    net_verlet = new Net(Vector3D(-200, -200, -1), Vector3D(200, 200, -1), num_rows, num_cols, config.mass, config.ks, {{num_rows, 0}, {num_rows, num_cols}});
#else
    net_euler = new Net(Vector3D(-0.5, -0.5, 0), Vector3D(0.5, 0.5, 0), num_rows, num_cols, config.mass, config.ks, {{num_rows, 0}, {num_rows, num_cols}});
    net_verlet = new Net(Vector3D(-0.5, -0.5, 0), Vector3D(0.5, 0.5, 0), num_rows, num_cols, config.mass, config.ks, {{num_rows, 0}, {num_rows, num_cols}});
#endif
}

void Application::destroy_scene()
{
    delete net_euler;
    delete net_verlet;
}

void Application::update()
{
    if (config.realtime)
    {
        static auto t_old = chrono::high_resolution_clock::now();
        auto t_now = chrono::high_resolution_clock::now();
        float t_elapsed = chrono::duration<float>(t_now - t_old).count();
        t_old = t_now;
        net_euler->SimulateEuler(t_elapsed, config.gravity);
        net_verlet->SimulateVerlet(t_elapsed, config.gravity);
    }
    else
    {
        for (int i = 0; i < config.steps_per_frame; i++)
        {
            net_euler->SimulateEuler(1 / config.steps_per_frame, config.gravity);
            net_verlet->SimulateVerlet(1 / config.steps_per_frame, config.gravity);
        }
    }
}

void Application::render()
{
    update();

    // Render ropes
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
        for (auto &v : object->vertices)
        {
            Vector3D p = v->position;
            glVertex2d(p.x, p.y);
        }
        glEnd();

        glBegin(GL_LINES);
        for (auto &s : object->springs)
        {
            Vector3D p1 = s->v1->position;
            Vector3D p2 = s->v2->position;
            glVertex2d(p1.x, p1.y);
            glVertex2d(p2.x, p2.y);
        }
        glEnd();

        glFlush();
    }
#else
    if (config.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    size_t size_v = max(net_euler->vertices.size(), net_verlet->vertices.size());
    auto vertices = new float[size_v][3];

    for (int i = 0; i < net_euler->vertices.size(); ++i)
    {
        vertices[i][0] = (float)net_euler->vertices[i]->position.x;
        vertices[i][1] = (float)net_euler->vertices[i]->position.y;
        vertices[i][2] = (float)net_euler->vertices[i]->position.z;
    }
    glUseProgram(shader_program_euler);
    glBindVertexArray(vao_euler);
    glBufferData(GL_ARRAY_BUFFER, net_euler->vertices.size() * 3 * sizeof(float), vertices, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawElements(GL_TRIANGLES, (GLsizei)net_euler->mesh.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    delete[] vertices;
    if (config.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

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

        ImGui::SameLine();
        if (ImGui::Button("Restart simulation"))
        {
            destroy_scene();
            create_scene();
        }

        ImGui::SameLine();
        if (ImGui::Button(("Switch to " + string(config.wireframe ? "normal" : "wireframe") + " mode").c_str()))
            config.wireframe = !config.wireframe;

        ImGui::End();
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
