// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(const Vector2f O, const Vector2f A, const Vector2f B, const Vector2f C)
{
    // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    Vector2f AB = B - A, BC = C - B, CA = A - C;
    Vector2f OA = A - O, OB = B - O, OC = C - O;
    float resA = OA.x() * CA.y() - OA.y() * CA.x(), resB = OB.x() * AB.y() - OB.y() * AB.x(), resC = OC.x() * BC.y() - OC.y() * BC.x();
    return resA > 0 && resB > 0 && resC > 0;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        // Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        // Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        // rasterize_triangle(t);
        rasterize_triangle_ssaa(t);
    }
    ssaa();
}

// Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();

    // TODO : Find out the bounding box of current triangle.
    // iterate through the pixel and find if the current pixel is inside the triangle
    int x_min = std::min(t.v[0].x(), std::min(t.v[1].x(), t.v[2].x()));
    int x_max = std::max(t.v[0].x(), std::max(t.v[1].x(), t.v[2].x())) + 1;
    int y_min = std::min(t.v[0].y(), std::min(t.v[1].y(), t.v[2].y()));
    int y_max = std::max(t.v[0].y(), std::max(t.v[1].y(), t.v[2].y())) + 1;
    x_min = std::max(0, x_min);
    x_max = std::min(width, x_max);
    y_min = std::max(0, y_min);
    y_max = std::min(height, y_max);

    for (int y = y_min;y < y_max;++y)
        for (int x = x_min; x < x_max; ++x)
            if (insideTriangle({ x + 0.5, y + 0.5 }, { t.v[0].x(), t.v[0].y() }, { t.v[1].x(), t.v[1].y() }, { t.v[2].x(), t.v[2].y() })) {
                // If so, use the following code to get the interpolated z value.
                auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                z_interpolated *= w_reciprocal;
                // TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
                if (z_interpolated < depth_buf[get_index(x, y)]) {
                    depth_buf[get_index(x, y)] = z_interpolated;
                    set_pixel({ float(x), float(y), z_interpolated}, t.getColor());
                }
            }
}

// This version of SSAA is wrong, the whole super-sampled image should have been stored.
/*void rst::rasterizer::rasterize_triangle_ssaa(const Triangle& t) {
    auto v = t.toVector4();

    int x_min = std::min(t.v[0].x(), std::min(t.v[1].x(), t.v[2].x()));
    int x_max = std::max(t.v[0].x(), std::max(t.v[1].x(), t.v[2].x())) + 1;
    int y_min = std::min(t.v[0].y(), std::min(t.v[1].y(), t.v[2].y()));
    int y_max = std::max(t.v[0].y(), std::max(t.v[1].y(), t.v[2].y())) + 1;
    x_min = std::max(0, x_min);
    x_max = std::min(width, x_max);
    y_min = std::max(0, y_min);
    y_max = std::min(height, y_max);
    for (int y = y_min; y < y_max; ++y)
        for (int x = x_min; x < x_max; ++x) {
            std::vector<std::vector<int>> inside = std::vector(2, std::vector(2, 0));
            std::vector<std::vector<float>> depth = std::vector(2, std::vector(2, .0f));
            for (int yy = 0; yy < 2; ++yy)
                for (int xx = 0; xx < 2; ++xx)
                    if (insideTriangle({ x + xx / 2. + 0.5, y + yy / 2. + 0.5 }, { t.v[0].x(), t.v[0].y() }, { t.v[1].x(), t.v[1].y() }, { t.v[2].x(), t.v[2].y() })) {
                        auto [alpha, beta, gamma] = computeBarycentric2D(x + xx / 2., y + yy / 2., t.v);
                        float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                        float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                        z_interpolated *= w_reciprocal;
                        if (z_interpolated < depth_buf[get_index(x, y)]) {
                            inside[yy][xx] = 1;
                            depth[yy][xx] = z_interpolated;
                        }
                    }
            int sum = inside[0][0] + inside[0][1] + inside[1][0] + inside[1][1];
            if (sum > 0) {
                float z_average = 0;
                for (int yy = 0; yy < 2; ++yy)
                    for (int xx = 0; xx < 2; ++xx)
                        if (inside[yy][xx] == 1)
                            z_average += depth[yy][xx];
                if (z_average < depth_buf[get_index(x, y)]) {
                    depth_buf[get_index(x, y)] = z_average;
                    set_pixel({ float(x), float(y), z_average }, t.getColor() * sum / 4);
                }
            }
        }
}*/

void rst::rasterizer::rasterize_triangle_ssaa(const Triangle& t) {
    auto v = t.toVector4();

    int x_min = std::min(t.v[0].x(), std::min(t.v[1].x(), t.v[2].x()));
    int x_max = std::max(t.v[0].x(), std::max(t.v[1].x(), t.v[2].x())) + 1;
    int y_min = std::min(t.v[0].y(), std::min(t.v[1].y(), t.v[2].y()));
    int y_max = std::max(t.v[0].y(), std::max(t.v[1].y(), t.v[2].y())) + 1;
    x_min = std::max(0, x_min);
    x_max = std::min(width, x_max);
    y_min = std::max(0, y_min);
    y_max = std::min(height, y_max);

    for (int y = y_min * 2; y < y_max * 2; ++y)
        for (int x = x_min * 2; x < x_max * 2; ++x)
            if (insideTriangle({ (x + 0.5) / 2, (y + 0.5) / 2 }, { t.v[0].x(), t.v[0].y() }, { t.v[1].x(), t.v[1].y() }, { t.v[2].x(), t.v[2].y() })) {
                auto [alpha, beta, gamma] = computeBarycentric2D(x / 2.0, y / 2.0, t.v);
                float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                z_interpolated *= w_reciprocal;
                if (z_interpolated < depth_buf_ssaa[get_index_ssaa(x, y)]) {
                    depth_buf_ssaa[get_index_ssaa(x, y)] = z_interpolated;
                    frame_buf_ssaa[get_index_ssaa(x, y)] = t.getColor();
                }
            }
}

void rst::rasterizer::ssaa() {
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            Vector3f color(0, 0, 0);
            color += frame_buf_ssaa[get_index_ssaa(x * 2, y * 2)];
            color += frame_buf_ssaa[get_index_ssaa(x * 2 + 1, y * 2)];
            color += frame_buf_ssaa[get_index_ssaa(x * 2, y * 2 + 1)];
            color += frame_buf_ssaa[get_index_ssaa(x * 2 + 1, y * 2 + 1)];
            color /= 4;
            frame_buf[get_index(x, y)] = color;
        }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
        std::fill(frame_buf_ssaa.begin(), frame_buf_ssaa.end(), Eigen::Vector3f{ 0, 0, 0 });
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
        std::fill(depth_buf_ssaa.begin(), depth_buf_ssaa.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    frame_buf_ssaa.resize(w * h * 4);
    depth_buf_ssaa.resize(w * h * 4);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height - 1 - y) * width + x;
}

int rst::rasterizer::get_index_ssaa(int x, int y)
{
    return (height * 2 - 1 - y) * width * 2 + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;
}

// clang-format on