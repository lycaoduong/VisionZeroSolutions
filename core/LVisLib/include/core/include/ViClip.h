#pragma once
#include "onnxruntime_cxx_api.h"
#include "opencv2/opencv.hpp"
#include <filesystem>

class ViCLIP
{
public:
    struct Result
    {
        std::string clsName;
        float score;
    };

public:
    ViCLIP(const OrtApi* ort, OrtSession* session);

public:
    cv::Mat iniTextuals(std::filesystem::path homeDir);
    cv::Mat getVisual(const cv::Mat frame);
    std::vector<float> getMatching(cv::Mat visualMat, cv::Mat dbTextualMat);
    Result getResult(std::vector<float> prediction);
    std::vector<float> finPerson(cv::Mat findMat, cv::Mat dbVisualMat);

private:
    std::vector<float> norm(std::vector<float> feature);
    std::vector<float> textualFeatures(std::string tokensPath);
    std::vector<float> softMax(cv::Mat logits_per_image);
    std::string getClassName(int argmaxVal);

private:
    cv::Mat textualMat;
    std::string textualName = "textual.txt";
    std::string textualDb = "textual.dat";
    std::vector<std::string> classes;

    int batch_size = 1;
    int channels = 3;
    int inpWidth = 224;  // Width of network's input image
    int inpHeight = 224; // Height of network's input image

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

    int feature_size = 768;
    std::vector<int64_t> outputDims{ batch_size, feature_size };
    size_t outputTensorSize = batch_size * feature_size;
    float scale = 1.0 / 114.54;
    cv::Scalar mean = cv::Scalar(122.77, 116.75, 104.094);

    std::vector<const char*> input_names{ "input" };
    std::vector<const char*> output_names{ "output" };
};

