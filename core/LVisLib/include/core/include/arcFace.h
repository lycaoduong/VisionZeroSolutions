#pragma once
#include "onnxruntime_cxx_api.h"
#include "opencv2/opencv.hpp"

class arcFace
{
public:

public:
    arcFace();
    void initModel(const OrtApi* ort, OrtSession* session);
    arcFace(const OrtApi* ort, OrtSession* session);
    std::vector<float> run_single_frame(cv::Mat face_img);
    double getMatchingScore(std::vector<float> featureA, std::vector<float> featureB);
    std::vector<double> getMatchingScores(std::vector<float> featureA, std::vector<std::vector<float>> featureBs);

private:
    double dotProduct(std::vector<float> fA, std::vector<float> fB);


private:
    int batch_size = 1;
    int channels = 3;
    int inpWidth = 112;  // Width of network's input image
    int inpHeight = 112; // Height of network's input image

    cv::Mat blob;
    cv::Size in_Size = cv::Size(inpWidth, inpHeight);

    const OrtApi* m_ort;
    OrtSession* m_session;
    OrtMemoryInfo* ort_memory_info;
    OrtValue* input_tensor;
    //OrtValue* output_tensor[1];


    std::vector<int64_t> m_vec_input_node_dims = { batch_size, channels, inpHeight, inpWidth };
    size_t m_s_input_tensor = batch_size * channels * inpWidth * inpHeight;
    std::vector<float> input_tensor_values;

    int feature_size = 512;
    std::vector<int64_t> outputDims{ batch_size, feature_size };
    size_t outputTensorSize = batch_size * feature_size;
    //std::vector<float> norm(std::vector<float> feature);

    float scale = 1.0 / 255.0;
    cv::Scalar mean = cv::Scalar(0, 0, 0);
};

