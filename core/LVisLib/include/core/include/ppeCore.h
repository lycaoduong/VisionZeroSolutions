#include "coreBase.h"
#include "return.h"

#include "modelLoader.h"
#include "yoloOnnxInfer.h"
#include "coreUtils.h"
#include "ViClip.h"
#include "BYTETracker.h"

// #define withCLIP
#define detectAll


class PPE
{
public:
    /*Stores information about detection(one box per frame)*/
    struct Result
    {
        DetectionList detections;
        DetectionList events;
    };

public:
    explicit PPE(fs::path clsNamePath, OnnxLoader::Model ppeModel);
    explicit PPE(fs::path clsNamePath, OnnxLoader::Model ppeModel, OnnxLoader::Model clipModel);
    void ensureInitialized();
    bool isTerminated() const;
    void terminate();

private:
    struct PPESetting
    {
        bool isActivated = false;
        bool withTrack = false;
        bool withClip = false;
        std::atomic<float> det_threshold{ 0.3 };
        std::atomic<float> nms_threshold{ 0.3 };
        std::atomic<float> normalize_size{ 10000.0 };

        std::vector<std::vector<cv::Point>> polygon_contours;
        std::vector<std::vector<cv::Point>> draw_contours;
    };
    PPESetting m_ppe_settings;

    struct PPETracker {
    ObjectTracker tracker;
    std::vector<cv::Rect> trackBoxes;
    int maxHis = 5;
    int trackID;
    };
    std::vector<PPETracker> trackers;

public:
    Result run(cv::Mat& frame);
    void setROIs(std::vector<std::string> roi_string);
    void setTrack(int maxTrack, int maxAge);
    void enableTracker(int frame_rate, int track_buffer);

private:
    void loadModel();
    Result runImpl(cv::Mat& frame);
    Result postProcessingV8withTrack(const cv::Mat prediction, cv::Mat cv_frame);
    Result postProcessingV8withByteTrack(const cv::Mat prediction, cv::Mat cv_frame);
    Result postProcessingV8(const cv::Mat prediction, cv::Mat cv_frame);
    void getTrack(std::vector<PPETracker> trackers, cv::Mat frame);
    bool check_inROI(cv::Point coor);
    int findMaxIoU(const cv::Rect& box, const std::vector<cv::Rect>& multi_boxes);
    float calculateIoU(const cv::Rect& box1, const cv::Rect& box2);
    void drawPrediction(cv::Mat& frame, Result result);
    void drawPredictionByteTrack(cv::Mat& frame, std::vector<STrack> stracks);
    std::vector<std::vector<cv::Point>> NormContour2contour(std::vector<std::vector<cv::Point>> cnts, cv::Mat frame);

private:
    bool m_netLoaded = false;
    bool m_terminated = false;
    const fs::path m_classMamePath;
    OnnxLoader::Model m_OnnxYoloModel;
    yoloOnnxInfer* m_OnnxYoloDetector;
    // fs::path detection_model_cls = fs::path("ppe23.names");
    std::vector<std::string> kClasses;
    std::vector<std::string> kClassesToDetect{"fire", "vehicle", "person"};
    // CLIP
    ViCLIP* mClip;
    OnnxLoader::Model m_OnnxCLipModel;
    cv::Mat m_DbtextualMat;

private:
    int m_maxTrack = 20;
    int m_maxTrackAge = 30;
    int m_maxMissTrack = 5;
    int trackCount = 0;
    int minTrackGetEvent = 1;
    std::vector<cv::Rect> trackBoxes;

    // Byte track
    BYTETracker tracker = BYTETracker(24, 30);
};
