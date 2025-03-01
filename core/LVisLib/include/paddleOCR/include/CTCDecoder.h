#ifndef CTC_DECODER_H
#define CTC_DECODER_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

class CTCDecoder {
private:
    std::vector<std::string> character;

public:
    CTCDecoder();

    int getNumClasses();
    // Decode from cv::Mat
    std::pair<std::string, std::vector<float>> operator()(const cv::Mat& output);

    std::pair<std::string, std::vector<float>> decode(
        const std::vector<int>& indices,
        const cv::Mat& outputs);
};

#endif // CTC_DECODER_H
