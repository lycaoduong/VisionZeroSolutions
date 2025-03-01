#pragma once
#include "coreBase.h"
#include "objectTracker.h"

class yoloOnnxInfer
{
public:
    struct Result
    {
        cv::Mat predictionMat;
    };

public:
    yoloOnnxInfer(); 
    //yoloOnnxInfer(const OrtApi* ort, OrtSession* session, fs::path classes_dir, int imgSize, int batchSize);
    yoloOnnxInfer(const OrtApi* ort, OrtSession* session, std::filesystem::path classes_dir, int imgSize, int batchSize);
    Result run_single_frame(cv::Mat frame);
    std::vector<float> run_batches(std::vector<cv::Mat> b_frame);
    void postprocess_v8(cv::Mat& frame, const cv::Mat& prediction, float confThreshold, float nmsThreshold);
    void postprocess_v4(cv::Mat& frame, const cv::Mat& prediction, float confThreshold, float nmsThreshold);
    void drawPred(int classId, float conf, int left, int top, int right, int bottom, cv::Mat& frame, cv::Scalar color);
    void drawPredTrack(int classId, float conf, int trackID, int left, int top, int right, int bottom, cv::Mat& frame, cv::Scalar color);
    //void initModel(const OrtApi* ort, OrtSession* session, fs::path classes_dir);
    void initModel(const OrtApi* ort, OrtSession* session, std::filesystem::path classes_dir);
    void initModel(const OrtApi* ort, OrtSession* session, int num_class);
    void setupForDynamicShapeModel(int64_t batch, int64_t img_width, int64_t img_height);
    void getTrack(std::vector<ObjectTracker> trackers, cv::Mat& frame);
    // void initTracker(cv::Mat& frame, std::vector<cv::Rect> boxes);
    std::vector<int64_t> getInputImgShape();
    std::vector<int64_t> getOutShape();
    std::vector<cv::Rect> getV8box(cv::Mat& frame, const cv::Mat& prediction, float confThreshold, float nmsThreshold);
    std::vector<cv::Rect> getV8boxWithTrack(cv::Mat& frame, const cv::Mat& prediction, float confThreshold, float nmsThreshold);
    void setTrack(int maxTrack, int maxAge);

private:
    //void load_cls_names(fs::path classes_dir);
    void load_cls_names(std::filesystem::path classes_dir);
    //void initParams(int imgSize, int batchSize);
    void CheckStatus(OrtStatus* status);
    void get_model_infor(const OrtApi* ort, OrtSession* session);
    void init_tensorSize();
    int findMaxIoU(const cv::Rect& box, const std::vector<cv::Rect>& multi_boxes);
    float calculateIoU(const cv::Rect& box1, const cv::Rect& box2);

public:
    std::vector<std::string> classes;

private:
    std::vector<ObjectTracker> trackers;
    std::string tracker_name = "CSRT";

private:
    const OrtApi* m_ort;
    OrtSession* m_session;
    OrtMemoryInfo* ort_memory_info;
    OrtValue* input_tensor;
    OrtValue* output_tensor[1];
    cv::Mat blob;

    std::vector<cv::Rect> trackBoxes;

    std::vector<cv::Rect> boxes;
    std::vector<int> indices;
    std::vector<cv::Rect> filterboxes;

    //OrtTypeInfo* typeinfo;

    // std::vector<std::string> classes;
    int numclass;

    std::vector<const char*> m_input_node_names;
    std::vector<const char*> m_output_node_names;

    size_t m_num_input_nodes;
    size_t m_num_input_dims;
    std::vector<int64_t> m_input_node_dims;
    size_t m_input_tensor_size = 1;
    std::vector<float> m_input_tensor_values;

    size_t m_num_output_nodes;
    size_t m_num_output_dims;
    std::vector<int64_t> m_output_node_dims;
    size_t m_output_tensor_size = 1;

    double m_mean = 1 / 255.0;
    cv::Scalar m_std = (0, 0, 0);

    int m_maxTrack = 30;
    int m_maxTrackAge = 60;
    int trackCount = 0;

};

