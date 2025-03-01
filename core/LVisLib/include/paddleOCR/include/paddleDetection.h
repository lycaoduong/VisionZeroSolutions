#ifndef paddleDetection_H
#define paddleDetection_H

#include <opencv2/opencv.hpp>
#include "onnxruntime_cxx_api.h"
#include <vector>
#include <string>


class paddleDetection {
public:
    explicit paddleDetection(const OrtApi* ort, OrtSession* session);
    std::vector<std::vector<cv::Point>> operator()(const cv::Mat& image);
    cv::Rect getOffsetBox(const cv::Mat& image, cv::Rect detBox, float offsetCoef);
    void updateSettings(float box_th, float mask_th);
    std::vector<cv::Point> sortedContour(std::vector<cv::Point> contour, float offsetCoef);
    cv::Mat fourPointTransform(const cv::Mat& image, const std::vector<cv::Point> detPoint);

private:
    double min_size = 3;
    double max_size = 960;
    double box_thresh = 0.4;
    double mask_thresh = 0.4;

    // cv::Mat mean = cv::Mat(1, 3, CV_64F, cv::Scalar(123.675, 116.28, 103.53));
    // cv::Mat std = cv::Mat(1, 3, CV_64F, cv::Scalar(1.0 / 58.395, 1.0 / 57.12, 1.0 / 57.375));

    float scale = 1.0 / 58.0;
    cv::Scalar mean = cv::Scalar(123.675, 116.28, 103.53);
    cv::Mat blob;

    const OrtApi* m_ort;
    OrtSession* m_session;
    OrtMemoryInfo* ort_memory_info;
    OrtValue* input_tensor;
    std::vector<float> input_tensor_values;


    cv::Mat resize(const cv::Mat& image);
    std::vector<std::vector<cv::Point>> filter_polygon(const std::vector<std::vector<cv::Point>>& points, const cv::Size& shape);
    std::vector<cv::Point> clip(const std::vector<cv::Point>& points, int h, int w);
    std::tuple<std::vector<std::vector<cv::Point>>, std::vector<float>> boxes_from_bitmap(const cv::Mat& output, const cv::Mat& mask, int dest_width, int dest_height);
    std::pair<std::vector<cv::Point>, double> get_min_boxes(const std::vector<cv::Point>& contour);
    double box_score(const cv::Mat& bitmap, const std::vector<cv::Point>& contour);
};

#endif // paddleDetection_H
