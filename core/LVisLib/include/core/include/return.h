#pragma once

struct Detection
{
    const cv::Rect2f boundingBox;
    const std::string classLabel;
    const float confidence;
    const std::string detail;
    const int trackID;
};
using DetectionList = std::vector<std::shared_ptr<Detection>>;