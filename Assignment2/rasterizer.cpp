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


static bool insideTriangle(int x, int y, const Vector3f* _v)
{   
    // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    float x_center = float(x) + 0.5f;
    float y_center = float(y) + 0.5f;
    Vector3f A = _v[0];
    Vector3f B = _v[1];
    Vector3f C = _v[2];

    Vector2f AP(x_center-A.x(), y_center-A.y());
    Vector2f BP(x_center-B.x(), y_center-B.y());
    Vector2f CP(x_center-C.x(), y_center-C.y());

    Vector2f AB(B.x()-A.x(), B.y()-A.y());
    Vector2f BC(C.x()-B.x(), C.y()-B.y());
    Vector2f CA(A.x()-C.x(), A.y()-C.y());

    float res1 = AP.x()*AB.y() - AP.y()*AB.x();
    float res2 = BP.x()*BC.y() - BP.y()*BC.x();
    float res3 = CP.x()*CA.y() - CP.y()*CA.x();

    if ((res1 >0 && res2>0 && res3>0)
            || (res1 <0 && res2<0 && res3<0)) {
        return true;
    }


    return false;
}

static bool insideTriangleMsaa(float x_center, float y_center, const Vector3f* _v)
{   

    Vector3f A = _v[0];
    Vector3f B = _v[1];
    Vector3f C = _v[2];

    Vector2f AP(x_center-A.x(), y_center-A.y());
    Vector2f BP(x_center-B.x(), y_center-B.y());
    Vector2f CP(x_center-C.x(), y_center-C.y());

    Vector2f AB(B.x()-A.x(), B.y()-A.y());
    Vector2f BC(C.x()-B.x(), C.y()-B.y());
    Vector2f CA(A.x()-C.x(), A.y()-C.y());

    float res1 = AP.x()*AB.y() - AP.y()*AB.x();
    float res2 = BP.x()*BC.y() - BP.y()*BC.x();
    float res3 = CP.x()*CA.y() - CP.y()*CA.x();

    if ((res1 >0 && res2>0 && res3>0)
            || (res1 <0 && res2<0 && res3<0)) {
        return true;
    }


    return false;
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
    outFile.open("outputMsaa4x_v2.txt");
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
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

        rasterize_triangle(t);
    }
    outFile.close();
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();
    
    // TODO : Find out the bounding box of current triangle.
    // iterate through the pixel and find if the current pixel is inside the triangle

    Vector4f A = v[0];
    Vector4f B = v[1];
    Vector4f C = v[2];

    // find bounding box
    float min_x = 1000.0, min_y = 1000.0, max_x = -1.0, max_y = -1.0;

    if (min_x > A.x()) min_x = A.x();
    if (min_x > B.x()) min_x = B.x();
    if (min_x > C.x()) min_x = C.x();

    if (min_y > A.y()) min_y = A.y();
    if (min_y > B.y()) min_y = B.y();
    if (min_y > C.y()) min_y = C.y();

    if (max_x < A.x()) max_x = A.x();
    if (max_x < B.x()) max_x = B.x();
    if (max_x < C.x()) max_x = C.x();

    if (max_y < A.y()) max_y = A.y();
    if (max_y < B.y()) max_y = B.y();
    if (max_y < C.y()) max_y = C.y();

    // bool debug_flag = true;

    // no msaa
    if (msaa == 0) { 
        for (int i = (int)min_x; i <= (int)max_x; i++) {
            for (int j = (int)min_y; j <= (int)max_y; j++) {
                if (insideTriangle(i, j, t.v)) {
                    auto[alpha, beta, gamma] = computeBarycentric2D(float(i), float(j), t.v);
                    float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    z_interpolated *= w_reciprocal;

                    z_interpolated *= -1.0f;
                    int index = j*width+i;
                    if (z_interpolated < depth_buf[index]) {
                        depth_buf[index] = z_interpolated;
                        Vector3f point(float(i), float(j), z_interpolated);
                        set_pixel(point, t.getColor());
                    }
                }
            }
        }
    // mass superSampling
    } else {
        float gap = 1.0f/(sqrt(msaa));
        for (int i = (int)min_x; i <= (int)max_x; i++) {
            for (int j = (int)min_y; j <= (int)max_y; j++) {
                int covered_sample = 0;
                Vector3f pixel_color(0, 0, 0);
                for (int m = 0; m < std::sqrt(msaa); m++)  {
                    for (int n = 0; n < std::sqrt(msaa); n++) {
                        float x = float(i)+ 1.0f/sqrt(msaa)/2 + m*gap;
                        float y = float(j)+ 1.0f/sqrt(msaa)/2 + n*gap;
                        int index = j*width*msaa + i*msaa + (n-1)*sqrt(msaa) + m;
                        if (insideTriangleMsaa(x, y, t.v)) {
                            auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                            float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                            float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                            z_interpolated *= w_reciprocal;
                            z_interpolated *= -1.0f;
                            
                            if (z_interpolated < msaa_depth_buf[index]) {
                                msaa_depth_buf[index] = z_interpolated;
                                msaa_frame_buf[index] = t.getColor();
                                covered_sample++;
                            }
                        }
                        pixel_color += msaa_frame_buf[index];
                    }
                }
                if (covered_sample != 0) {
                    float covered_ratio = (float)covered_sample/msaa;

                    // if (covered_ratio != 1) {
                    //     outFile << "x:" << i << " y: " << j << "  covered_sample: " << covered_sample << std::endl; 
                    //     outFile << "color: " << t.getColor() << "  covered_ratio: " << covered_ratio << std::endl;
                    //     outFile << "t.v[0]: " << v[0](0) << " " << (700-v[0][1]) << std::endl;
                    // }
                    

                    Vector3f point(float(i), float(j), -1/*no use, random number*/);

                    // how to solve cover replacement? should replace but not add!
                    set_msaa_pixel(point, pixel_color/msaa);
                }
                
            }
        }
    }
    

    // If so, use the following code to get the interpolated z value.
    //auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
    //float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
    //float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
    //z_interpolated *= w_reciprocal;

    // TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
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
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
        std::fill(msaa_depth_buf.begin(), msaa_depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h, int msaa) : width(w), height(h), msaa(msaa)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    msaa_depth_buf.resize(w*h*msaa);
    msaa_frame_buf.resize(w*h*msaa);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

void rst::rasterizer::set_msaa_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on