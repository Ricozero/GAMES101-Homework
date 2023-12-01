#include <iostream>

#include <imgui.h>
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#include "application.h"

// #define USE_2D

Application::Application(Config config, Viewer *viewer): config(config), viewer(viewer)
{
    memset(durations, 0, sizeof(durations));
    first_drag = true;
    yaw = 0;
    pitch = 0;
    scale = 1;
}

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

        uniform int screenWidth;
        uniform int screenHeight;
        uniform mat4 view;
        uniform float scale;

        // In OpenGL, camera space is left-handed, z+ pointing from camera towards front
        float aspectRatio = float(screenWidth) / screenHeight;
        float t = 400, b = -t, r = t * aspectRatio, l = -r, n = -r, f = r;
        mat4 orth = mat4(
            2/(r-l), 0, 0, -(r+l)/(r-l),
            0, 2/(t-b), 0, -(t+b)/(t-b),
            0, 0, -2/(f-n), -(f+n)/(f-n),
            0, 0, 0, 1
        );

        mat4 model = mat4(
            scale, 0, 0, 0,
            0, scale, 0, 0,
            0, 0, scale, 0,
            0, 0, 0, 1
        );

        void main()
        {
            vec4 pos = vec4(aPos.x, aPos.y, aPos.z, 1.0);
            gl_Position = orth * view * model * pos;
        }
    )delimiter";
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        cout << infoLog << endl;
    }

    int fragment_shader_euler = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragment_shader_source_euler = R"delimiter(
        #version 330 core
        out vec4 fragColor;

        void main()
        {
            fragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
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
        out vec4 fragColor;

        void main()
        {
            fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
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

    glBindVertexArray(vao_verlet);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_verlet);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_verlet);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, net_verlet->mesh.size() * sizeof(int), net_verlet->mesh.data(), GL_STATIC_DRAW);

    float view[4][4]{0};
    for (int i = 0; i < 4; ++i)
        view[i][i] = 1;
    int view_location = glGetUniformLocation(shader_program_euler, "view");
    glUseProgram(shader_program_euler);
    glUniformMatrix4fv(view_location, 1, GL_TRUE, (GLfloat*)view);
    glUseProgram(shader_program_verlet);
    glUniformMatrix4fv(view_location, 1, GL_TRUE, (GLfloat*)view);

    int scale_location = glGetUniformLocation(shader_program_euler, "scale");
    glUseProgram(shader_program_euler);
    glUniform1f(scale_location, 1);
    glUseProgram(shader_program_verlet);
    glUniform1f(scale_location, 1);
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
    net_euler = new Net(Vector3D(-200, -200, -100), Vector3D(200, 200, -100), num_rows, num_cols, config.mass, config.k1, config.k2, config.k3, {{num_rows, 0}, {num_rows, num_cols}});
    net_verlet = new Net(Vector3D(-200, -200, 100), Vector3D(200, 200, 200), num_rows, num_cols, config.mass, config.k1, config.k2, config.k3, {{num_rows, 0}, {num_rows, num_cols}});
}

void Application::destroy_scene()
{
    delete net_euler;
    delete net_verlet;
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
    glEnable(GL_DEPTH_TEST);
    if (config.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    size_t size_v = max(net_euler->vertices.size(), net_verlet->vertices.size());
    auto vertices = new float[size_v][3];

    if (config.render_euler)
    {
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
    }

    if (config.render_verlet)
    {
        for (int i = 0; i < net_verlet->vertices.size(); ++i)
        {
            vertices[i][0] = (float)net_verlet->vertices[i]->position.x;
            vertices[i][1] = (float)net_verlet->vertices[i]->position.y;
            vertices[i][2] = (float)net_verlet->vertices[i]->position.z;
        }
        glUseProgram(shader_program_verlet);
        glBindVertexArray(vao_verlet);
        glBufferData(GL_ARRAY_BUFFER, net_verlet->vertices.size() * 3 * sizeof(float), vertices, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glDrawElements(GL_TRIANGLES, (GLsizei)net_verlet->mesh.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    delete[] vertices;
    if (config.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
#endif
}

void Application::render_config_window()
{
    const static int SLIDER_WIDTH = 150;

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
        ImGui::Text("3D only options:");
        ImGui::Text("Euler:");
        ImGui::SameLine(); ImGui::Checkbox("Render##euler", &config.render_euler);
        ImGui::SameLine(); ImGui::Checkbox("Simulate##euler", &config.simulate_euler);
        ImGui::Text("Verlet:");
        ImGui::SameLine(); ImGui::Checkbox("Render##verlet", &config.render_verlet);
        ImGui::SameLine(); ImGui::Checkbox("Simulate##verlet", &config.simulate_verlet);
        if (ImGui::Button(("Switch to " + string(config.wireframe ? "normal" : "wireframe") + " mode").c_str()))
            config.wireframe = !config.wireframe;
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
    int width_location, height_location;
    width_location = glGetUniformLocation(shader_program_euler, "screenWidth");
    height_location = glGetUniformLocation(shader_program_euler, "screenHeight");
    glUseProgram(shader_program_euler);
    glUniform1i(width_location, (GLint)screen_width);
    glUniform1i(height_location, (GLint)screen_height);
    glUseProgram(shader_program_verlet);
    glUniform1i(width_location, (GLint)screen_width);
    glUniform1i(height_location, (GLint)screen_height);
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
    int view_location = glGetUniformLocation(shader_program_euler, "view");
    glUseProgram(shader_program_euler);
    glUniformMatrix4fv(view_location, 1, GL_TRUE, (GLfloat*)view);
    glUseProgram(shader_program_verlet);
    glUniformMatrix4fv(view_location, 1, GL_TRUE, (GLfloat*)view);
#endif
}

void Application::scroll_event(float offset_x, float offset_y)
{
#ifndef USE_2D
    float sensitivity = 0.05f;
    scale *= (1 + sensitivity * offset_y);
    int view_location = glGetUniformLocation(shader_program_euler, "scale");
    glUseProgram(shader_program_euler);
    glUniform1f(view_location, (GLfloat)scale);
    glUseProgram(shader_program_verlet);
    glUniform1f(view_location, (GLfloat)scale);
#endif
}

string Application::info()
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "%.1f/%.1f/%.1f", durations[0], durations[1], durations[2]);
    return buf;
}