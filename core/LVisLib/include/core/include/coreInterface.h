#include <opencv2/core/core.hpp>
#pragma once
void drawPred(std::string className, float conf, cv::Point TL, cv::Point RB, cv::Mat& frame, cv::Scalar color);
