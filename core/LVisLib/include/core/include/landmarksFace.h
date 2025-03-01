#pragma once
#include "onnxruntime_cxx_api.h"
#include "opencv2/opencv.hpp"

typedef struct
{
	cv::Rect rect;
	float prob;
	std::vector<cv::Point> kpt;
} Face;

class lmkFace
{ 
public:
    lmkFace();
    lmkFace(const OrtApi* ort, OrtSession* session);
    void initModel(const OrtApi* ort, OrtSession* session);
    std::vector<cv::Mat> run_single_frame(cv::Mat srcimg);
    
private:
    void generate_proposal(cv::Mat out, std::vector<cv::Rect>& boxes, std::vector<float>& confidences, std::vector< std::vector<cv::Point>>& landmarks, int imgh,int imgw, float ratioh, float ratiow, int padh, int padw);
    cv::Mat ConvertTo3DMat(const float* data, int depth, int rows, int cols);
    void softmax_(const float* x, float* y, int length);
    float sigmoid_x(float x);
    cv::Mat resize_image(cv::Mat srcimg, int *newh, int *neww, int *padh, int *padw);


private:
    int batch_size = 1;
    int channels = 3;
    int inpWidth = 640;  // Width of network's input image
    int inpHeight = 640; // Height of network's input image

    cv::Mat blob;
    cv::Size in_Size = cv::Size(inpWidth, inpHeight);

    const OrtApi* m_ort;
    OrtSession* m_session;
    OrtMemoryInfo* ort_memory_info;
    OrtValue* input_tensor;


    std::vector<int64_t> m_vec_input_node_dims = { batch_size, channels, inpHeight, inpWidth };
    size_t m_s_input_tensor = batch_size * channels * inpWidth * inpHeight;
    std::vector<float> input_tensor_values;

    // Based on Yolov8-face landmarks
    float scale = 1.0 / 255.0;
    cv::Scalar mean = cv::Scalar(0, 0, 0);
    std::vector<int> fpnSize{80, 40, 20};
    int outChannel = 80;
    const int num_class = 1;  
	const int reg_max = 16;
    float confThreshold = 0.3;
    float nmsThreshold = 0.4;
    const bool keep_ratio = true;

    std::vector<const char*> input_names = { "images" };
    std::vector<const char*> output_names = { "output0", "389", "397" };
};
