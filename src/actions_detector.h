#pragma once

#include "DB.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>

#include <nx/sdk/helpers/uuid_helper.h>
#include <nx/sdk/uuid.h>
#include "detection.h"
#include "frame.h"
#include "geometry.h"

#include "modelLoader/include/modelLoader.h"
#include "core/include/yoloOnnxInfer.h"
#include "core/include/ViClip.h"
#include "byteTrack/include/BYTETracker.h"

#define withCLIP

namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {
            namespace fs = std::filesystem;
            class ActionDetector
            {
            public:
                explicit ActionDetector(fs::path modelPath,
                                        OnnxLoader::Model m_det_model);

                explicit ActionDetector(fs::path modelPath,
                         OnnxLoader::Model m_det_model,
                         OnnxLoader::Model m_clip_model,
                         bool isLoadTextualDataFromDB);

                void ensureInitialized();
                bool isTerminated() const;
                void terminate();
                void setConf(float confTh);
                void setROIs(std::vector<std::string> roi_string);
                void initTracker(int frame_rate, int track_buffer);
                DetectionList run(const Frame& frame);
                DetectionList runTest(cv::Mat frame);

            public:
                void drawPrediction(cv::Mat& frame, DetectionList result);

                //loading textual data from DB
                bool AddAction(std::string actionName, std::string& errorMsg);
                bool RemoveAction(std::string actionName, std::string& errorMsg);
                std::vector<std::string> ReadActions();

            private:

                struct CoreSetting
                {
                    bool isActivated = false;
                    std::atomic<float> det_threshold{ 0.3 };
                    std::atomic<float> nms_threshold{ 0.3 };
                    std::atomic<float> normalize_size{ 10000.0 };

                    std::vector<std::vector<cv::Point>> polygon_contours;
                    std::vector<std::vector<cv::Point>> draw_contours;
                };
                CoreSetting m_core_settings;

                //loading textual data from DB
                void LoadTextualData();
                std::pair <std::string, float> GetResult(std::vector<float> confidences);

            private:
                DetectionList runImpl(const Frame& frame);
                DetectionList postProcessingV8(const cv::Mat prediction);
                DetectionList postProcessingV8withTrack(const cv::Mat prediction, const cv::Mat cv_frame);
                DetectionList postProcessingV8withByteTrack(const cv::Mat prediction, cv::Mat cv_frame);
                void loadModel();
                float calculateIoU(const cv::Rect& box1, const cv::Rect& box2);
                int findMaxIoU(const cv::Rect& box, const std::vector<cv::Rect>& multi_boxes);
                bool check_inROI(cv::Point coor);
                void getTrack(std::vector<ObjectTracker> trackers, cv::Mat frame);
                int findPosition(const std::vector<int>& trackIDList, int trackID);

            private:
                
                std::vector<ObjectTracker> trackers;
                std::vector<cv::Rect> trackBoxes;
                int m_maxTrack = 20;
                int m_maxTrackAge = 30; // max track Frame
                int maxTrackList = 30; // max track person
                int m_maxMissTrack = 5;
                int trackCount = 0;
                std::vector<int> trackersMissCount;

                // Byte Track
                nx::sdk::Uuid m_trackId = nx::sdk::UuidHelper::randomUuid();
                std::vector<int> trackIdList;
                std::vector<nx::sdk::Uuid> trackIdsNXWitness;


            private:
                bool m_netLoaded = false;
                bool m_terminated = false;
                const fs::path m_modelPath;

            private:
                OnnxLoader::Model m_OnnxYoloModel;
                yoloOnnxInfer* m_OnnxYoloDetector;
                fs::path detection_model_cls = fs::path("coco.names");

                ViCLIP* mClip;
                OnnxLoader::Model m_OnnxCLipModel;
                double offsetFactor = 0.1;
                cv::Mat m_DbtextualMat;

                // Byte track with fix 5 FPS, max age 30 frame
                BYTETracker tracker = BYTETracker(5, 30);

                //loading textual data from DB
                DataBase* db = NULL;
                std::string iniFilename;
                std::vector<std::string> m_classNames;
                bool m_isLoadTextualDataFromDB;

            };
        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx

