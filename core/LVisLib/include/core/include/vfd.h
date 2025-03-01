// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once
#include "coreBase.h"

#include "return.h"

#define HAS_CPP17
#ifdef HAS_CPP17 // Use fs::
#include <filesystem>
namespace fs = std::filesystem;
#else //HAS_CPP14
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif 


class VFD
{
public:
    /*Stores information about detection(one box per frame)*/
    struct Result
    {
        DetectionList detections;
        DetectionList events;
    };

public:
    // VFD(const OrtApi* ort, OrtSession* session);
    VFD(std::vector<std::string> classe_names, std::vector<int64_t> input_node_dims);
    Result getResult(cv::Mat& frame, const cv::Mat prediction);
    void drawPrediction(cv::Mat& frame, Result result);
    void On_Off_Core(bool isactivated);
    void setConf(float fire_conf, float smoke_conf);
    void setROI(std::vector<std::string> roi_string);
    void setFPS(double fps);
    void setSnapshotDir(std::string snapDir);
    std::string getCurrentCameraName();
    void setCameraName(std::string oldName, std::string newName);
    // bool isTerminated() const;

private:

    struct VFDSetting
    {
        bool isActivated = false;
        std::atomic<float> fire_threshold{ 0.3 };
        std::atomic<float> smoke_threshold{ 0.3 };
        std::atomic<float> nms_threshold{ 0.3 };
        std::atomic<float> detect_fill_rate{ 0.3 };

        std::atomic<double> m_fps{ 5.0 };
        std::atomic<double> nVFDEventCooldownTime{ 20.0 }; //second

        //std::vector<std::string> polygon_rois;

        std::vector<std::vector<cv::Point>> polygon_contours;
        std::vector<std::vector<cv::Point>> draw_contours;

        //std::vector<std::vector<cv::Point2f>> polygon_contours = {
        //    {cv::Point2f(0.0, 0.0), cv::Point2f(1.0, 0.0), cv::Point2f(1.0, 1.0), cv::Point2f(0.0, 1.0)},
        //    {cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0)},
        //    {cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0)},
        //    {cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0), cv::Point2f(0.0, 0.0)},
        //    //{cv::Point2f(0.0, 0.0), cv::Point2f(1.0, 0.0), cv::Point2f(1.0, 1.0), cv::Point2f(0.0, 1.0)}
        //};

        std::vector<int> m_nVFDEventCooldown = { -1, -1, -1, -1 };

        std::string deviceName = "Camera1";
        std::string coreName = "VFD";
        std::string snapShotname = "VisioninSnapshots";
        std::string ori_fd = "Origin";
        std::string event_fd = "Event";
        std::string snapShotdir = "D:/";

    };
    VFDSetting m_vfdsetting;


private:
    bool check_inROI(cv::Point coor);
    //DetectionList get_event(DetectionList& object);
    DetectionList detections, events;

    DetectionList get_event_ly(DetectionList& detections);
    void vfd_event_updated(std::string cls);
    void saveEvent(cv::Mat ori_frame, cv::Mat event_frame, std::string detType);

private:
    // Core internal Params
    int m_nFireQueeuLen = 30;
    int m_nSmokeQueeuLen = 30;
    int m_nFireEventCooldown = -1;
    int m_nSmokeEventCooldown = -1;
    std::deque<int> queueFireTracker;
    std::deque<int> queueSmokeTracker;
    char buff[500];
    int aFireTrack;
    int aSmokeTrack;
    int sum_up;

    std::vector<std::string> m_classes;
    std::vector<int64_t> m_input_node_dims;

    std::vector<cv::Rect> boxes;
    std::vector<int> indices;
    int probability_index = 4;
    float normalize_size = 10000.0;
    char save_name[5000];
};
