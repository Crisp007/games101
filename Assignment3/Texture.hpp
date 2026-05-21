//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <eigen3/Eigen/Eigen>
#include <opencv2/opencv.hpp>
class Texture{
private:
    cv::Mat image_data;

public:
    Texture(const std::string& name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    int width, height;

    Eigen::Vector3f getColor(float u, float v)
    {
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

    Eigen::Vector3f getColorBilinear(float u, float v)
    {
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
  
        int i = (int)(u_img-0.5f);
        int j = (int)(v_img-0.5f);
        if (i < 0 || j < 0) {
            // this case, no bileaner algorithm
            auto color = image_data.at<cv::Vec3b>(v_img, u_img);
            return Eigen::Vector3f(color[0], color[1], color[2]);
        }

        auto color00 = image_data.at<cv::Vec3b>(j, i);
        auto color10 = image_data.at<cv::Vec3b>(j, std::min(i+1, width-1));
        auto color01 = image_data.at<cv::Vec3b>(std::min(j+1, height-1), i);
        auto color11 = image_data.at<cv::Vec3b>(std::min(j+1, height-1), std::min(i+1, width-1));
        Eigen::Vector3f f00(color00[0], color00[1], color00[2]);
        Eigen::Vector3f f01(color01[0], color01[1], color01[2]);
        Eigen::Vector3f f10(color10[0], color10[1], color10[2]);
        Eigen::Vector3f f11(color11[0], color11[1], color11[2]);

        float s = u_img-(float(i)+0.5f);
        float t = v_img-(float(j)+0.5f);

        Eigen::Vector3f row1 = (1.0f-s)*f01 + s*f11;
        Eigen::Vector3f row2 = (1.0f-s)*f00 + s*f10;
        Eigen::Vector3f result_color = (1.0f-t)*row2 + t*row1;

        return result_color;
    }

};
#endif //RASTERIZER_TEXTURE_H
