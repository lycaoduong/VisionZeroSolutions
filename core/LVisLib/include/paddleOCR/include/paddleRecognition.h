#ifndef paddleRecognition_H
#define paddleRecognition_H

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "CTCDecoder.h"

class paddleRecognition {

public:
    explicit paddleRecognition(const OrtApi* ort, OrtSession* session);

    cv::Mat resize(const cv::Mat& image, float max_wh_ratio);

    std::string run_single(const cv::Mat image);

    // std::pair<std::vector<std::string>, std::vector<std::vector<float>>> run(const std::vector<cv::Mat>& images);

private:
    std::vector<int64_t> input_shape = {3, 48, 320};
    CTCDecoder ctc_decoder;


    float scale = 1.0 / 255.0;
    cv::Scalar mean = cv::Scalar(0.5, 0.5, 0.5);
    cv::Mat blob;

    const OrtApi* m_ort;
    OrtSession* m_session;
    OrtMemoryInfo* ort_memory_info;
    OrtValue* input_tensor;
    std::vector<float> input_tensor_values;

};

#endif // paddleRecognition_H
