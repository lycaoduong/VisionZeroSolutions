
#include "actions_detector.h"
#include <opencv2/core.hpp>
#include "exceptions.h"
#include <opencv2/core/cuda.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "core/include/coreUtils.h"


namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            using namespace std::string_literals;

            using namespace cv;
            using namespace cv::dnn;

            ActionDetector::ActionDetector(fs::path modelPath,
                                     OnnxLoader::Model m_det_model,
                                     OnnxLoader::Model m_clip_model,
                                     bool isLoadTextualDataFromDB) :
                m_modelPath(std::move(modelPath)),
                m_OnnxYoloModel(std::move(m_det_model)),
                m_OnnxCLipModel(std::move(m_clip_model)),
                m_isLoadTextualDataFromDB(isLoadTextualDataFromDB)
            {
                if (m_isLoadTextualDataFromDB) {
                    iniFilename = m_modelPath.string() + "/nx_plugin_clip.ini";
                    db = new DataBase(ReadINI("DB", "db_ip_address", iniFilename), /* DB hostname*/
                        ReadINI("DB", "db_user_id", iniFilename), /* DB user ID*/
                        ReadINI("DB", "db_user_password", iniFilename), /* DB user Password*/
                        ReadINI("DB", "db_name", iniFilename), /* DB name*/
                        std::atoi(ReadINI("DB", "db_port", iniFilename).c_str())); /* DB port*/
                    LoadTextualData();
                }
                ensureInitialized();
            }


            /**
             * Load the model if it is not loaded, do nothing otherwise. In case of errors terminate the
             * plugin and throw a specialized exception.
             */
            void ActionDetector::ensureInitialized()
            {
                if (isTerminated())
                {
                    throw ActionDetectorIsTerminatedError(
                        "Object detector initialization error: object detector is terminated.");
                }
                if (m_netLoaded)
                    return;

                try
                {
                    loadModel();
                }
                catch (const cv::Exception& e)
                {
                    terminate();
                    throw ActionDetectorInitializationError("Loading model: " + cvExceptionToStdString(e));
                }
                catch (const std::exception& e)
                {
                    terminate();
                    throw ActionDetectorInitializationError("Loading model: Error: "s + e.what());
                }
            }

            bool ActionDetector::isTerminated() const
            {
                return m_terminated;
            }

            void ActionDetector::terminate()
            {
                m_terminated = true;
            }

            void ActionDetector::setConf(float confTh)
            {
                m_core_settings.det_threshold = confTh;
            }

            void ActionDetector::setROIs(std::vector<std::string> roi_string)
            {
                m_core_settings.polygon_contours.clear();
                for (int i = 0; i < roi_string.size(); ++i)
                {
                    if (roi_string[i] == "")
                    {
                        m_core_settings.polygon_contours.push_back({});
                    }
                    else
                    {
                        m_core_settings.polygon_contours.push_back(roi2contour(roi_string[i]));
                    }
                }
            }

            void ActionDetector::initTracker(int frame_rate, int track_buffer)
            {
                tracker = BYTETracker(frame_rate, track_buffer);
            }

            DetectionList ActionDetector::run(const Frame& frame)
            {
                //if (isTerminated())
                //    throw ActionDetectorIsTerminatedError("Detection error: object detector is terminated.");

                try
                {
                    return runImpl(frame);
                }
                catch (const cv::Exception& e)
                {
                    terminate();
                    throw ObjectDetectionError(cvExceptionToStdString(e));
                }
                catch (const std::exception& e)
                {
                    terminate();
                    throw ObjectDetectionError("Error: "s + e.what());
                }
            }


            void ActionDetector::loadModel()
            {
                //static const auto modelConfiguration = m_modelPath /
                //    fs::path("haarcascade_frontalface_alt.xml");

                //face_cascade.load(modelConfiguration.u8string());

                //fs::path OnnxYolo_names_path = m_modelPath / detection_model_cls;
                std::filesystem::path OnnxYolo_names_path = m_modelPath / detection_model_cls;
                m_OnnxYoloDetector = new yoloOnnxInfer();
                m_OnnxYoloDetector->initModel(m_OnnxYoloModel.m_ort, m_OnnxYoloModel.m_session, OnnxYolo_names_path);

#ifdef withCLIP
                mClip = new ViCLIP(m_OnnxCLipModel.m_ort, m_OnnxCLipModel.m_session);
                // Init textual.txt and textual.dat from Plugin Folder, texttual embeddings was stored to m_DbtextualMat
                if (!m_isLoadTextualDataFromDB)
                    m_DbtextualMat = mClip->iniTextuals(m_modelPath);
#endif // withCLIP

                m_netLoaded = true;
            }


            std::shared_ptr<Detection> convertRawDetectionToDetection(
                const Mat& rawDetections,
                int detectionIndex,
                const nx::sdk::Uuid trackId)
            {
                enum class OutputIndex
                {
                    classIndex = 1,
                    confidence = 2,
                    xBottomLeft = 3,
                    yBottomLeft = 4,
                    xTopRight = 5,
                    yTopRight = 6,
                };
                static constexpr float confidenceThreshold = 0.5F; //< Chosen arbitrarily.

                const int& i = detectionIndex;
                const float confidence = rawDetections.at<float>(i, (int)OutputIndex::confidence);
                const auto classIndex = (int)(rawDetections.at<float>(i, (int)OutputIndex::classIndex));
                const std::string classLabel = kClasses[(size_t)classIndex];
                const bool confidentDetection = confidence >= confidenceThreshold;
                bool oneOfRequiredClasses = std::find(
                    kClassesToDetect.begin(), kClassesToDetect.end(), classLabel) != kClassesToDetect.end();
                if (confidentDetection && oneOfRequiredClasses)
                {
                    const float xBottomLeft = rawDetections.at<float>(i, (int)OutputIndex::xBottomLeft);
                    const float yBottomLeft = rawDetections.at<float>(i, (int)OutputIndex::yBottomLeft);
                    const float xTopRight = rawDetections.at<float>(i, (int)OutputIndex::xTopRight);
                    const float yTopRight = rawDetections.at<float>(i, (int)OutputIndex::yTopRight);
                    const float width = xTopRight - xBottomLeft;
                    const float height = yTopRight - yBottomLeft;

                    return std::make_shared<Detection>(Detection{
                        /*boundingBox*/ nx::sdk::analytics::Rect(xBottomLeft, yBottomLeft, width, height),
                        classLabel,
                        confidence,
                        trackId
                        });
                }
                return nullptr;
            }

            float ActionDetector::calculateIoU(const cv::Rect& box1, const cv::Rect& box2) {
                // Calculate the intersection rectangle
                cv::Rect intersection = box1 & box2;

                // If there is no intersection, the area is zero
                if (intersection.area() <= 0) {
                    return 0.0f;
                }

                // Calculate the union area
                float union_area = static_cast<float>(box1.area() + box2.area() - intersection.area());

                // Calculate IoU
                return intersection.area() / union_area;
            }

            int ActionDetector::findMaxIoU(const cv::Rect& box, const std::vector<cv::Rect>& multi_boxes) {
                float max_iou = 0.0f;
                int max_index = -1;

                for (size_t i = 0; i < multi_boxes.size(); ++i) {
                    float iou = calculateIoU(box, multi_boxes[i]);
                    if (iou > max_iou) {
                        max_iou = iou;
                        max_index = static_cast<int>(i);
                    }
                }

                return max_index;
            }

            bool ActionDetector::check_inROI(cv::Point coor)
            {
                bool inRoi = false;
                for (std::vector<cv::Point> cnt : m_core_settings.polygon_contours)
                {
                    //float x = (float)coor.x;
                    //float y = (float)coor.y;
                    //std::cout << coor.x << '-' << coor.y;
                    //cv::Point2f coor_scale(x, y);
                    if (cv::pointPolygonTest(cnt, coor, false) > 0)
                    {
                        inRoi = true;
                        break;
                    }
                }
                return inRoi;
            }

            void ActionDetector::LoadTextualData()
            {
                std::string errorMsg = "";
                auto p = db->ReadRowsDB(errorMsg);

                m_classNames = p.first;

                int classNum = p.first.size(); // ex: 3
                int featureSize = p.second.size() / classNum; // ex: 768
                std::vector<float> allTextualFeatures = p.second; // ex: 3 x 768 = 2,304 elements

                const cv::Mat floatMat(
                    /*_rows*/ classNum,
                    /*_cols*/ featureSize,
                    /*_type*/ CV_32F, //CV_32F
                    /*_s*/ allTextualFeatures.data());

                m_DbtextualMat = floatMat.clone();
            }

            std::pair<std::string, float> ActionDetector::GetResult(std::vector<float> confidences)
            {
                std::pair <std::string, float> p;
                std::size_t classSize = m_classNames.size();
                std::size_t confidSize = confidences.size();

                if (!(classSize > 0 && confidSize > 0 && classSize == confidSize))
                    return p;

                int index = std::distance(confidences.begin(), std::max_element(confidences.begin(), confidences.end()));
                p.first = m_classNames[index]; //class name
                p.second = *std::max_element(confidences.begin(), confidences.end());; //confidence

                return p;
            }

            DetectionList ActionDetector::runImpl(const Frame& frame)
            {
                const Mat cv_mat = frame.cvMat;
                
                auto predictionMat = m_OnnxYoloDetector->run_single_frame(cv_mat);

                DetectionList results;

                results = postProcessingV8withByteTrack(predictionMat.predictionMat, cv_mat);

                //frame_count += 1;
                return results;
            }

            DetectionList ActionDetector::runTest(cv::Mat frame)
            {
                auto predictionMat = m_OnnxYoloDetector->run_single_frame(frame);

                DetectionList results;

                results = postProcessingV8withByteTrack(predictionMat.predictionMat, frame);

                //frame_count += 1;
                return results;
            }

            void ActionDetector::drawPrediction(cv::Mat& frame, DetectionList result)
            {
                // Draw ROIs
                if (m_core_settings.polygon_contours.size() > 0)
                {
                    m_core_settings.draw_contours = NormContour2contour(m_core_settings.polygon_contours, frame);
                    cv::drawContours(frame, m_core_settings.draw_contours, -1, cv::Scalar(0, 255, 0), 5);
                }

                for (const std::shared_ptr<Detection>& detection : result)
                {
                    auto idx = detection->IdxID;
                    idx += 3;
                    auto boxColor = cv::Scalar(37 * idx % 255, 17 * idx % 255, 29 * idx % 255);

                    auto nxbox = detection->boundingBox;
                    auto box = nxRectToCvRect(nxbox, frame.cols, frame.rows);
                    auto conf = detection->confidence;

                    int x = (int)(box.x);
                    int y = (int)(box.y);
                    int w = (int)(box.width);
                    int h = (int)(box.height);
                    std::string label = cv::format("%s: %d", detection->classLabel, detection->IdxID);
#ifdef withCLIP
                    auto state = detection->clip_state;
                    //std::cout << state << std::endl;
                    label += ". " + state;
#endif
                    cv::putText(frame, label, cv::Point(x, y - 5),
                        0, 0.6, boxColor, 2, cv::LINE_AA);
                    cv::rectangle(frame, cv::Rect(x, y, w, h), boxColor, 2);
                }
            }

            bool ActionDetector::AddAction(std::string actionName, std::string& errorMsg)
            {
                if (db->InsertRowDB(actionName, errorMsg)) {
                    LoadTextualData(); // update textual data
                    return true;
                }
                return false;
            }

            bool ActionDetector::RemoveAction(std::string actionName, std::string& errorMsg)
            {
                if (db->DeleteRowDB(actionName, errorMsg)) {
                    LoadTextualData(); // update textual data
                    return true;
                }
                return false;
            }

            std::vector<std::string> ActionDetector::ReadActions()
            {
                return m_classNames;
            }

            void ActionDetector::getTrack(std::vector<ObjectTracker> trackers, cv::Mat frame)
            {
                trackBoxes.clear();
                for (ObjectTracker tracker : trackers)
                {
                    auto trackDone = tracker.update(frame);
                    if (trackDone)
                    {
                        trackBoxes.push_back(tracker.getTrackBox());
                    }
                    else
                    {
                        trackBoxes.push_back(cv::Rect(-1, -1, -1, -1));
                    }  
                }
            }

            int ActionDetector::findPosition(const std::vector<int>& trackIDList, int trackID)
            {
                auto it = std::find(trackIDList.begin(), trackIDList.end(), trackID);
                if (it != trackIDList.end()) {
                    return std::distance(trackIDList.begin(), it); // Calculate the index
                }
                return -1; // Not found
            }

            DetectionList ActionDetector::postProcessingV8withByteTrack(const cv::Mat prediction, const cv::Mat cv_frame)
            {
                DetectionList result;

                std::vector<int> classIds;
                std::vector<float> confidences;
                std::vector<cv::Rect> boxes;

                for (size_t i = 0; i < prediction.rows; i++)
                {
                    double confidence;

                    const int probability_size = prediction.cols - 4; // Base YoloV8 structure: probability_index
                    const float* prob_array_ptr = &prediction.at<float>(i, 4);
                    size_t objectClass = std::max_element(prob_array_ptr, prob_array_ptr + probability_size) - prob_array_ptr;
                    confidence = prediction.at<float>(i, (int)objectClass + 4);

                    if (confidence >= m_core_settings.det_threshold)
                    {
                        // Yolo V8 format
                        int center_x = (int)(prediction.at<float>(i, 0) * cv_frame.cols / float(m_OnnxYoloDetector->getInputImgShape()[2]));
                        int center_y = (int)(prediction.at<float>(i, 1) * cv_frame.rows / float(m_OnnxYoloDetector->getInputImgShape()[3]));

                        int width = (int)(prediction.at<float>(i, 2) * cv_frame.cols / float(m_OnnxYoloDetector->getInputImgShape()[2]));
                        int height = (int)(prediction.at<float>(i, 3) * cv_frame.rows / float(m_OnnxYoloDetector->getInputImgShape()[3]));

                        int left = center_x - (width / 2);
                        int top = center_y - (height / 2);

                        classIds.push_back(objectClass);
                        confidences.push_back((float)confidence);

                        boxes.push_back(cv::Rect(left, top, width, height));
                    }
                }

                std::vector<int> indices;
                cv::dnn::NMSBoxes(boxes, confidences, m_core_settings.det_threshold, m_core_settings.nms_threshold, indices);

                // Track List
                std::vector<Object> objects;

                for (int i = 0; i < indices.size(); ++i)  // classIds
                {
                    int idx = indices[i];
                    cv::Rect box = boxes[idx];
                    int clsId = classIds[idx];
                    float confidence = confidences[idx];
                    const std::string classLabel = kClasses[clsId];

                    int normCX = (int)((box.x + (box.width / 2)) * m_core_settings.normalize_size / cv_frame.cols);
                    int normCY = (int)((box.y + (box.height / 2)) * m_core_settings.normalize_size / cv_frame.rows);

                    if (check_inROI(cv::Point(normCX, normCY)))
                    {
                        bool oneOfRequiredClasses = std::find(
                            kClassesToDetect.begin(), kClassesToDetect.end(), classLabel) != kClassesToDetect.end();
                        if (oneOfRequiredClasses)
                        {
                            cv::Rect_<float> rect((float)box.x, (float)box.y, (float)box.width, (float)box.height);
                            Object obj{ rect, clsId, confidence };
                            objects.push_back(obj);
                        }
                    }
                }

                // track
                std::vector<STrack> output_stracks = tracker.update(objects);

                for (int i = 0; i < output_stracks.size(); i++)
                {
                    std::vector<float> tlwh = output_stracks[i].tlwh;
                    float conf = output_stracks[i].score;
                    int id = output_stracks[i].track_id;
                    std::string clsName = kClasses[output_stracks[i].labelID];

                    if (tlwh[2] * tlwh[3] > 20)
                    {
                        std::string state;

                        auto box = cv::Rect2f(tlwh[0], tlwh[1], tlwh[2], tlwh[3]);

                        int position = findPosition(trackIdList, id);
                        if (position != -1) {
                            //std::cout << value << " found at position " << position << ".\n";
                            m_trackId = trackIdsNXWitness[position];
                        }
                        else {
                            //std::cout << value << " not found in the vector.\n";
                            trackIdList.push_back(id);
                            m_trackId = nx::sdk::UuidHelper::randomUuid();
                            trackIdsNXWitness.push_back(m_trackId);

                            if (trackIdList.size() > maxTrackList) {
                                trackIdList.erase(trackIdList.begin());
                                trackIdsNXWitness.erase(trackIdsNXWitness.begin());
                            }
                        }

#ifdef withCLIP
                        // Calculate the offset (% increase)
                        int offsetWidth = static_cast<int>(box.width * offsetFactor);
                        int offsetHeight = static_cast<int>(box.height * offsetFactor);

                        // Expand the rectangle
                        int newX = std::max((int)box.x - offsetWidth, 0);
                        int newY = std::max((int)box.y - offsetHeight, 0);
                        int newWidth = std::min((int)box.width + 2 * offsetWidth, cv_frame.cols - newX);
                        int newHeight = std::min((int)box.height + 2 * offsetHeight, cv_frame.rows - newY);

                        cv::Rect expandedRect(newX, newY, newWidth, newHeight);

                        // Crop the image
                        auto cloneFrame = cv_frame.clone();
                        cv::Mat croppedImage = cloneFrame(expandedRect);

                        // Perform CLIP
                        auto visualEmb = mClip->getVisual(croppedImage);
                        // Get Cosine distance between current visual vector and Textual Db Matrix
                        auto prediction = mClip->getMatching(visualEmb, m_DbtextualMat);
                        // Find max score between current visual vector and Textual Db Matrix
                        //auto clipResult = mClip->getResult(prediction);
                        //state = clipResult.clsName;
                        auto p = GetResult(prediction);
                        state = p.first;
#else
                        state = "None";
#endif // withCLIP
                        auto nxRect = cvRectToNxRect(box, cv_frame.cols, cv_frame.rows);
                        const std::shared_ptr<Detection> detection = std::make_shared<Detection>(Detection{
                            /*boundingBox*/ nxRect,
                            /*className*/ clsName,
                            /*confidence*/ conf,
                            /*trackID*/ m_trackId,
                            /*IdxID*/ id,
                            /*clipState*/ state
                            });

                        if (detection)
                        {
                            result.push_back(detection);
                            /*return result;*/
                        }
                    }
                }
                return result;
            }


            DetectionList ActionDetector::postProcessingV8withTrack(const cv::Mat prediction, const cv::Mat cv_frame)
            {
                DetectionList result;

                std::vector<int> classIds;
                std::vector<float> confidences;
                std::vector<cv::Rect> boxes;

                for (size_t i = 0; i < prediction.rows; i++)
                {
                    double confidence;

                    const int probability_size = prediction.cols - 4; // Base YoloV8 structure: probability_index
                    const float* prob_array_ptr = &prediction.at<float>(i, 4);
                    size_t objectClass = std::max_element(prob_array_ptr, prob_array_ptr + probability_size) - prob_array_ptr;
                    confidence = prediction.at<float>(i, (int)objectClass + 4);

                    if (confidence >= m_core_settings.det_threshold)
                    {
                        // Yolo V8 format
                        int center_x = (int)(prediction.at<float>(i, 0) * cv_frame.cols / float(m_OnnxYoloDetector->getInputImgShape()[2]));
                        int center_y = (int)(prediction.at<float>(i, 1) * cv_frame.rows / float(m_OnnxYoloDetector->getInputImgShape()[3]));

                        int width = (int)(prediction.at<float>(i, 2) * cv_frame.cols / float(m_OnnxYoloDetector->getInputImgShape()[2]));
                        int height = (int)(prediction.at<float>(i, 3) * cv_frame.rows / float(m_OnnxYoloDetector->getInputImgShape()[3]));

                        int left = center_x - (width / 2);
                        int top = center_y - (height / 2);

                        classIds.push_back(objectClass);
                        confidences.push_back((float)confidence);

                        boxes.push_back(cv::Rect(left, top, width, height));
                    }
                }

                std::vector<int> indices;
                cv::dnn::NMSBoxes(boxes, confidences, m_core_settings.det_threshold, m_core_settings.nms_threshold, indices);

                // Get track
                getTrack(trackers, cv_frame);

                for (int i = 0; i < indices.size(); ++i)  // classIds
                {
                    int idx = indices[i];
                    cv::Rect box = boxes[idx];
                    int clsId = classIds[idx];
                    float confidence = confidences[idx];
                    const std::string classLabel = kClasses[clsId];

                    int normCX = (int)((box.x + (box.width / 2)) * m_core_settings.normalize_size / cv_frame.cols);
                    int normCY = (int)((box.y + (box.height / 2)) * m_core_settings.normalize_size / cv_frame.rows);

                    if (check_inROI(cv::Point(normCX, normCY)))
                    {
                        bool oneOfRequiredClasses = std::find(
                            kClassesToDetect.begin(), kClassesToDetect.end(), classLabel) != kClassesToDetect.end();
                        if (oneOfRequiredClasses)
                        {
                            auto matchTrackIOU = findMaxIoU(box, trackBoxes);
                            std::string state;

                            if (matchTrackIOU == -1)
                            {
                                ObjectTracker tracker;
                                tracker.setMaxAge(m_maxTrackAge);
                                tracker.initialize(cv_frame, box);

                                matchTrackIOU = trackCount % m_maxTrack;

                                if (trackers.size() < m_maxTrack)
                                {
                                    m_trackId = nx::sdk::UuidHelper::randomUuid();
                                    trackIdsNXWitness.push_back(m_trackId);
                                    trackers.push_back(tracker);
                                }
                                else
                                {
                                    m_trackId = nx::sdk::UuidHelper::randomUuid();
                                    trackIdsNXWitness[matchTrackIOU] = m_trackId;
                                    trackers[matchTrackIOU] = tracker;
                                }

                                trackCount += 1;

                                // Using CLIP to get Person State
#ifdef withCLIP
                                // Calculate the offset (% increase)
                                int offsetWidth = static_cast<int>(box.width * offsetFactor);
                                int offsetHeight = static_cast<int>(box.height * offsetFactor);

                                // Expand the rectangle
                                int newX = std::max(box.x - offsetWidth, 0);
                                int newY = std::max(box.y - offsetHeight, 0);
                                int newWidth = std::min(box.width + 2 * offsetWidth, cv_frame.cols - newX);
                                int newHeight = std::min(box.height + 2 * offsetHeight, cv_frame.rows - newY);

                                cv::Rect expandedRect(newX, newY, newWidth, newHeight);

                                // Crop the image
                                auto cloneFrame = cv_frame.clone();
                                cv::Mat croppedImage = cloneFrame(expandedRect);

                                // Perform CLIP
                                auto visualEmb = mClip->getVisual(croppedImage);
                                // Get Cosine distance between current visual vector and Textual Db Matrix
                                auto prediction = mClip->getMatching(visualEmb, m_DbtextualMat);
                                // Find max score between current visual vector and Textual Db Matrix
                                //auto clipResult = mClip->getResult(prediction);
                                //state = clipResult.clsName;
                                auto p = GetResult(prediction);
                                state = p.first;
#else
                                state = "None";
#endif // withCLIP
                            }
                            else
                            {
                                m_trackId = trackIdsNXWitness[matchTrackIOU];
                            }

                            auto nxRect = cvRectToNxRect(box, cv_frame.cols, cv_frame.rows);
                            const std::shared_ptr<Detection> detection = std::make_shared<Detection>(Detection{
                                /*boundingBox*/ nxRect,
                                /*className*/ classLabel,
                                /*confidence*/ confidence,
                                /*trackID*/ m_trackId,
                                /*IdxID*/ matchTrackIOU,
                                /*clipState*/ state
                                });

                            if (detection)
                            {
                                result.push_back(detection);
                                /*return result;*/
                            }
                        }
                    }
                }
                return result;
            }

            DetectionList ActionDetector::postProcessingV8(const cv::Mat prediction)
            {
                DetectionList result;

                std::vector<int> classIds;
                std::vector<float> confidences;
                std::vector<cv::Rect> boxes;

                for (size_t i = 0; i < prediction.rows; i++)
                {
                    double confidence;

                    const int probability_size = prediction.cols - 4; // Base YoloV8 structure: probability_index
                    const float* prob_array_ptr = &prediction.at<float>(i, 4);
                    size_t objectClass = std::max_element(prob_array_ptr, prob_array_ptr + probability_size) - prob_array_ptr;
                    confidence = prediction.at<float>(i, (int)objectClass + 4);

                    if (confidence >= m_core_settings.det_threshold)
                    {
                        // Yolo V8 format
                        int center_x = (int)(prediction.at<float>(i, 0) * m_core_settings.normalize_size / float(m_OnnxYoloDetector->getInputImgShape()[2]));
                        int center_y = (int)(prediction.at<float>(i, 1) * m_core_settings.normalize_size / float(m_OnnxYoloDetector->getInputImgShape()[3]));

                        int width = (int)(prediction.at<float>(i, 2) * m_core_settings.normalize_size / float(m_OnnxYoloDetector->getInputImgShape()[2]));
                        int height = (int)(prediction.at<float>(i, 3) * m_core_settings.normalize_size / float(m_OnnxYoloDetector->getInputImgShape()[3]));

                        int left = center_x - (width / 2);
                        int top = center_y - (height / 2);

                        classIds.push_back(objectClass);
                        confidences.push_back((float)confidence);

                        boxes.push_back(cv::Rect(left, top, width, height));
                    }
                }

                std::vector<int> indices;
                cv::dnn::NMSBoxes(boxes, confidences, m_core_settings.det_threshold, m_core_settings.nms_threshold, indices);

                for (int i = 0; i < indices.size(); ++i)  // classIds
                {
                    int idx = indices[i];
                    cv::Rect box = boxes[idx];
                    int clsId = classIds[idx];
                    float confidence = confidences[idx];
                    const std::string classLabel = kClasses[clsId];


                    if (check_inROI(cv::Point(box.x + (box.width / 2), box.y + (box.height / 2))))
                    {
                        bool oneOfRequiredClasses = std::find(
                            kClassesToDetect.begin(), kClassesToDetect.end(), classLabel) != kClassesToDetect.end();
                        if (oneOfRequiredClasses)
                        {
                            //auto nxRect = cvRectToNxRect(box, cv_frame.cols, cv_frame.rows);
                            auto nxRect = cvRectToNxRect(box, m_core_settings.normalize_size, m_core_settings.normalize_size);
                            nx::sdk::Uuid m_trackId = nx::sdk::UuidHelper::randomUuid();

                            const std::shared_ptr<Detection> detection = std::make_shared<Detection>(Detection{
                                /*boundingBox*/ nxRect,
                                classLabel,
                                confidence,
                                m_trackId
                                });

                            if (detection)
                            {
                                result.push_back(detection);
                                /*return result;*/
                            }
                        }
                    }
                }
                return result;
            }

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx