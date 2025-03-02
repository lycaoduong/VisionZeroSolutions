// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "device_agent.h"

#include <chrono>
#include <exception>

#include <opencv2/core.hpp>

#include <nx/sdk/analytics/helpers/event_metadata.h>
#include <nx/sdk/analytics/helpers/event_metadata_packet.h>
#include <nx/sdk/analytics/helpers/object_metadata.h>
#include <nx/sdk/analytics/helpers/object_metadata_packet.h>

#include <nx/sdk/helpers/string.h>

#include <nx/sdk/helpers/settings_response.h>
#include <nx/kit/utils.h>
#include <nx/kit/debug.h>
#include <nx/kit/json.h>

#include "detection.h"
#include "exceptions.h"
#include "frame.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>


namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            using namespace nx::sdk;
            using namespace nx::sdk::analytics;

            /**
             * @param deviceInfo Various information about the related device, such as its id, vendor, model,
             *     etc.
             */
            DeviceAgent::DeviceAgent(const nx::sdk::IDeviceInfo* deviceInfo,
                                     fs::path pluginHomeDir,
                                     Engine* engine):
                // Call the DeviceAgent helper class constructor telling it to verbosely report to stderr.
                ConsumingDeviceAgent(deviceInfo, /*enableOutput*/ true)
            {
                m_devAgent = new DeviceAgentModel(pluginHomeDir.string());
                cameraName = deviceInfo->name();

                m_engine = engine;
                m_engine->WritePluginLog("DeviceAgent::DeviceAgent(): The plugin has been ENABLED on the following camera : " + cameraName);

                // Load Detection model
                if (loadModel(pluginHomeDir / m_DetmodelName, m_Detmodel))
                    m_engine->WritePluginLog("Engine::Engine() : detection model loaded successfully. (location: " + fs::path(pluginHomeDir / m_DetmodelName).string() + ")");
                else
                    m_engine->WritePluginLog("Engine::Engine() : failed to load detection model. (location: " + fs::path(pluginHomeDir / m_DetmodelName).string() + ")");

                // Load CLIP Visual Model
                if (loadModel(pluginHomeDir / m_ClipVisualmodelName, m_ClipVisualmodel))
                    m_engine->WritePluginLog("Engine::Engine() : CLIP Visual model loaded successfully. (location: " + fs::path(pluginHomeDir / m_ClipVisualmodelName).string() + ")");
                else
                    m_engine->WritePluginLog("Engine::Engine() : failed to load CLIP Visual model. (location: " + fs::path(pluginHomeDir / m_ClipVisualmodelName).string() + ")");

                //Create Action Detection Object (will load actions from the DB)
                m_ActionDetector = new ActionDetector(pluginHomeDir, m_Detmodel, m_ClipVisualmodel, true);
                // Init Tracker
                m_ActionDetector->initTracker(m_deviceAgentSettings.assumedFPS, m_deviceAgentSettings.trackBuffer);

                m_pluginHomeDir = pluginHomeDir;
                m_iniFilename = m_pluginHomeDir.string() + "/nx_plugin_clip.ini";

                rest.Init(m_iniFilename);
            }
            
            DeviceAgent::~DeviceAgent()
            {
                videoWriter.release();
                m_engine->WritePluginLog("DeviceAgent::~DeviceAgent(): The plugin has been DISABLED on the following camera : " + cameraName);
            }

            /**
             *  @return JSON with the particular structure. Note that it is possible to fill in the values
             * that are not known at compile time, but should not depend on the DeviceAgent settings.
             */
            std::string DeviceAgent::manifestString() const
            {
                // Tell the Server that the plugin can generate the events and objects of certain types.
                // Id values are strings and should be unique. Format of ids:
                // `{vendor_id}.{plugin_id}.{event_type_id/object_type_id}`.
                //
                // See the plugin manifest for the explanation of vendor_id and plugin_id.

                std::string manifestStr;
                //main object
                rapidjson::Document doc;
                doc.SetObject();
                rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

                //event types
                rapidjson::Document newCLIPEventManifest;
                newCLIPEventManifest.SetObject();
                rapidjson::Document::AllocatorType& allocatorCLIPEvent = newCLIPEventManifest.GetAllocator();
                newCLIPEventManifest.AddMember("id", rapidjson::Value(kPersonEventType.c_str(), allocatorCLIPEvent), allocatorCLIPEvent);
                newCLIPEventManifest.AddMember("name", rapidjson::Value("New Action Detected", allocatorCLIPEvent), allocatorCLIPEvent);

                //object types

                // License Plate
                rapidjson::Document CLIPObjectManifest;
                CLIPObjectManifest.SetObject();
                rapidjson::Document::AllocatorType& allocatorCLIPObject = CLIPObjectManifest.GetAllocator();
                CLIPObjectManifest.AddMember("id", rapidjson::Value(kPersonObjectType.c_str(), allocatorCLIPObject), allocatorCLIPObject);
                CLIPObjectManifest.AddMember("name", rapidjson::Value("Person", allocatorCLIPObject), allocatorCLIPObject);

                //eventTypes array
                rapidjson::Document eventTypes, objectTypes;
                eventTypes.SetArray();
                objectTypes.SetArray();

                eventTypes.PushBack(rapidjson::Value(newCLIPEventManifest, allocator), allocator);

                objectTypes.PushBack(rapidjson::Value(CLIPObjectManifest, allocator), allocator);

                doc.AddMember("eventTypes", eventTypes, allocator);
                doc.AddMember("objectTypes", objectTypes, allocator);

                //conversion: rapidjson document -> string
                rapidjson::StringBuffer buff;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writter(buff);
                doc.Accept(writter);
                manifestStr = buff.GetString();

                return manifestStr;
            }

            /**
             * Called when the Server sends a new uncompressed frame from a camera.
             */
            bool DeviceAgent::pushUncompressedVideoFrame(const IUncompressedVideoFrame* videoFrame)
            {
                m_terminated = m_terminated || m_ActionDetector->isTerminated();
                if (m_terminated)
                {
                    if (!m_terminatedPrevious)
                    {
                        pushPluginDiagnosticEvent(
                            IPluginDiagnosticEvent::Level::error,
                            "Plugin is in broken state.",
                            "Disable the plugin.");
                        m_terminatedPrevious = true;
                    }
                    return true;
                }

                // Make sure FPS per camera is 5
                if (m_deviceAgentSettings.time_us_from_camera != 0) {
                    if (videoFrame->timestampUs() != m_deviceAgentSettings.time_us_from_camera)
                    {
                        m_deviceAgentSettings.camFPS = 1000000 / (videoFrame->timestampUs() - m_deviceAgentSettings.time_us_from_camera);
                    }
                    else
                    {
                        m_deviceAgentSettings.camFPS = 30;
                    }
                    m_deviceAgentSettings.skipFrame = m_deviceAgentSettings.camFPS / m_deviceAgentSettings.assumedFPS;
                }
                m_deviceAgentSettings.time_us_from_camera = videoFrame->timestampUs();

                if (m_deviceAgentSettings.skipFrame != 0)
                {
                    if (m_frameIndex % m_deviceAgentSettings.skipFrame == 0)
                    {
                        const MetadataPacketList metadataPackets = processFrame(videoFrame);
                        for (const Ptr<IMetadataPacket>& metadataPacket : metadataPackets)
                        {
                            metadataPacket->addRef();
                            pushMetadataPacket(metadataPacket.get());
                        }
                    }
                }

                ++m_frameIndex;

                return true; //< There were no errors while processing the video frame.
            }

            /**
             * Serves the similar purpose as pushMetadataPacket(). The differences are:
             * - pushMetadataPacket() is called by the plugin, while pullMetadataPackets() is called by Server.
             * - pushMetadataPacket() expects one metadata packet, while pullMetadataPacket expects the
             *     std::vector of them.
             *
             * There are no strict rules for deciding which method is "better". A rule of thumb is to use
             * pushMetadataPacket() when you generate one metadata packet and do not want to store it in the
             * class field, and use pullMetadataPackets otherwise.
             */

            void DeviceAgent::doSetNeededMetadataTypes(
                //nx::sdk::Result<void>* /*outValue*/,
                nx::sdk::Result<void>* outValue,
                const nx::sdk::analytics::IMetadataTypes* /*neededMetadataTypes*/)
            {
                if (m_terminated)
                    return;

                try
                {
                    m_ActionDetector->ensureInitialized();
                }
                catch (const ActionDetectorInitializationError& e)
                {
                    *outValue = { ErrorCode::otherError, new String(e.what()) };
                    m_terminated = true;
                }
                catch (const ActionDetectorIsTerminatedError& /*e*/)
                {
                    m_terminated = true;
                }
            }

            Ptr<ObjectMetadataPacket> DeviceAgent::detectionsToObjectMetadataPacket(
                const DetectionList& detections,
                int64_t timestampUs,
                const Frame& frame)
            {
                if (detections.empty())
                    //pushPluginDiagnosticEvent(
                    //    IPluginDiagnosticEvent::Level::warning,
                    //    "Object detection warning.",
                    //    "No detection box");
                    return nullptr;

                const auto objectMetadataPacket = makePtr<ObjectMetadataPacket>();

                for (const std::shared_ptr<Detection>& detection : detections)
                {
                    const auto objectMetadata = makePtr<ObjectMetadata>();

                    objectMetadata->setBoundingBox(detection->boundingBox);
                    objectMetadata->setConfidence(detection->confidence);
                    //objectMetadata->setTrackId(detection->trackId);

                    // Convert class label to object metadata type id.

                    if (detection->classLabel == "person") 
                    {
                        objectMetadata->setTypeId(kPersonObjectType);
                        objectMetadata->setTrackId(detection->trackId);
                        // Display Tracking IDs
                        objectMetadata->addAttribute(nx::sdk::makePtr<Attribute>(IAttribute::Type::string, "ID: ", std::to_string(detection->IdxID)));
                        // If CLIP state is not default and not empty, show Person state on each Person Object 
                        if ((!detection->clip_state.empty()) && (detection->clip_state != "default"))
                        {
                            objectMetadata->addAttribute(nx::sdk::makePtr<Attribute>(IAttribute::Type::string, "Status: ", detection->clip_state)); 

                            //extract tracking ID from event detection
                            std::string trackID = std::to_string(detection->IdxID);
                            int index = Find_ID(trackingBuffer, trackID);
                            if (index == -1) {
                                trackingBuffer.push_back(trackID);

                                std::vector<std::string> eventInfo{
                                   detection->classLabel /* event name */,
                                   cameraName, /* device name */
                                   std::to_string(detection->boundingBox.x), /* bounding box x coordinate */
                                   std::to_string(detection->boundingBox.y), /* bounding box y coordinate */
                                   std::to_string(detection->boundingBox.width), /* bounding box width */
                                   std::to_string(detection->boundingBox.height), /* bounding box height */
                                   std::to_string(timestampUs) /* event timestamp */
                                };
                                cv::Mat image = frame.cvMat.clone();
                                auto p1 = std::make_pair(eventInfo, image);

                                procThread = std::thread(&Rest::ProcessEventImage, &rest, p1);
                                procThread.detach();

                                //external system connection (play IP Speaker sound, turn ON Signal tower)
                                auto p2 = GetExternalSystemCodes(detection->clip_state);
                                if(m_engine->connectToSignalTower)
                                    ControlSignalTower(p2.first, detection->clip_state);
                                if (m_engine->connectToSpeaker)
                                    PlaySpeakerSound(p2.second, detection->clip_state);
                            }
                            if (trackingBuffer.size() == maxTrackingBuffer) {
                                trackingBuffer.erase(trackingBuffer.begin());
                            }

                        }
                    }
                    // There is no "else", because only the detections with those types are generated.

                    objectMetadataPacket->addItem(objectMetadata.get());
                }

                objectMetadataPacket->setTimestampUs(timestampUs);
                objectMetadataPacket->setDurationUs(detDurationUs);

                return objectMetadataPacket;
            }

            DeviceAgent::MetadataPacketList DeviceAgent::eventsToEventMetadataPacketList(
                const DetectionList& events,
                int64_t timestampUs,
                const Frame& frame)
            {
                if (events.empty())
                    return {};

                MetadataPacketList result;

                const auto objectDetectedEventMetadataPacket = makePtr<EventMetadataPacket>();

                for (const std::shared_ptr<Detection>& event : events)
                {
                    const auto eventMetadata = makePtr<EventMetadata>();
                    // Convert class label to object metadata type id.
                    if ((event->classLabel == "person") && (event->clip_state != "default")) // prevent "default" action from being displayed to the user
                    {
                        eventMetadata->setTypeId(kPersonEventType);
                    }
                    objectDetectedEventMetadataPacket->addItem(eventMetadata.get());
                }

                objectDetectedEventMetadataPacket->setTimestampUs(timestampUs);
                result.push_back(objectDetectedEventMetadataPacket);

                return result;
            }


            DeviceAgent::MetadataPacketList DeviceAgent::processFrame(
                const IUncompressedVideoFrame* videoFrame)
            {
                const Frame frame(videoFrame, m_frameIndex);

                if (m_deviceAgentSettings.saveVideo)
                {
                    cv::resize(frame.cvMat, saveFrame, saveVideoSize, 0, 0, cv::INTER_LINEAR);
                    videoWriter.write(saveFrame);
                }

                try
                {
                    // Get Detection
                    DetectionList detections = m_ActionDetector->run(frame);

                    const auto& objectMetadataPacket =
                        detectionsToObjectMetadataPacket(detections, frame.timestampUs, frame);

                  /*  const auto& eventMetadataPacketList =
                        eventsToEventMetadataPacketList(detections, frame.timestampUs, frame);*/

                    MetadataPacketList result;

                    if (objectMetadataPacket)
                        result.push_back(objectMetadataPacket);

                  /*  result.insert(
                        result.end(),
                        std::make_move_iterator(eventMetadataPacketList.begin()),
                        std::make_move_iterator(eventMetadataPacketList.end())
                    );*/

                    return result;
                }
                catch (const ObjectDetectionError& e)
                {
                    pushPluginDiagnosticEvent(
                        IPluginDiagnosticEvent::Level::error,
                        "Object detection error.",
                        e.what());
                    m_terminated = true;
                }

                return {};
            }

            bool DeviceAgent::loadModel(fs::path model_dir, OnnxLoader::Model& onnxModel)
            {
                OnnxLoader* modelLoader = new OnnxLoader();
                // For encrypt model
                void const* mBuffer;
                int bSize;
                decryptModel(model_dir.string().c_str(), mBuffer, bSize);

                bool isLoaded = modelLoader->loadModel(mBuffer, bSize, "cuda", "0", onnxModel);
                return isLoaded;
            }

            //add new action to the DB
            void DeviceAgent::AddAction(std::string actionName)
            {
                if (actionName == "default") {
                    //prevent user from adding the "default" action. 
                    //This action is automatically added by the system, and not it is not meant to be controlled by the user.
                    pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error,
                        "ADD ACTION STATUS",
                        "failed to add '" + actionName + "'" + " action. \nReason: It is a system reserved action. Please, choose a different name.");
                    return;
                }

                std::string errorMsg = "";
                if (m_ActionDetector->AddAction(actionName, errorMsg)) {
                    pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::info,
                        "ADD ACTION STATUS",
                        "new '" + actionName + "'" + " action has been added.");
                }
                else {
                    pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error,
                        "ADD ACTION STATUS",
                        "failed to add '" + actionName + "'" + " action.\nReason: " + errorMsg);
                }
            }

            //remove existing action from the DB
            void DeviceAgent::RemoveAction(std::string actionName)
            {
                if (actionName == "default") {
                    //prevent user from deleting the "default" action to guarantee continuous system operation
                    //this will prevent the program from stopping suddendly when all other actions are removed by the user from the DB
                    pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error,
                        "DELETE ACTION STATUS",
                        "failed to delete '" + actionName + "'" + " action. \nReason: It is a system reserved action. Please, choose a different name.");
                    return;
                }

                std::string errorMsg = "";
                if (m_ActionDetector->RemoveAction(actionName, errorMsg)) {
                    pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::info,
                        "DELETE ACTION STATUS",
                        "'" + actionName + "'" + " action has been deleted.");
                }
                else {
                    pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error,
                        "DELETE ACTION STATUS",
                        "failed to delete '" + actionName + "'" + " action.\nReason: " + errorMsg);
                }
            }

            //display current action list on the notifications tab
            void DeviceAgent::ShowActionList()
            { 
                std::vector<std::string> actionList = m_ActionDetector->ReadActions();
                pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::info,
                    "ACTION LIST STATUS",
                    "# actions = '" + std::to_string(actionList.size()) + "'\n#action names = " + CombineStrings(actionList));
            }

            void DeviceAgent::ControlSignalTower(std::string ledCode, std::string eventName)
            {
                if (ledCode.empty() || std::strlen(ledCode.c_str()) != 6 || eventName.empty())
                    return;

                std::string outStr;

                m_restThread.Run(
                    m_pluginHomeDir.string(),
                    "",
                    m_engine->sigtowerResourcePath,
                    cameraName,
                    m_engine->sigtowerIpAddress,
                    m_engine->sigtowerPort,
                    "",
                    "",
                    eventName,
                    ledCode, 
                    std::vector<std::string>(), 
                    RestAPI::PluginOperationType::CONTROL_SIGNAL_TOWER,
                    outStr,
                    0.0, 
                    0.0, 
                    0.0,
                    0.0, 
                    0.0, 
                    0.0, 
                    0.0,
                    0,   
                    std::map<std::string, std::string>{},
                    "",
                    0,
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    ""
                    );
            }

            void DeviceAgent::PlaySpeakerSound(std::uint32_t audioFileNumber, std::string eventName)
            {
                if (audioFileNumber < 1 || audioFileNumber > 50 || eventName.empty())
                    return;

                std::string outStr;

                m_restThread.Run(
                    m_pluginHomeDir.string(),
                    "",
                    "",
                    cameraName,
                    m_engine->speakerIpAddress,
                    m_engine->spearkerPort,
                    m_engine->spearkerUserID,
                    m_engine->spearkerUserPasssword,
                    eventName,
                    std::to_string(audioFileNumber),
                    std::vector<std::string>(),
                    RestAPI::PluginOperationType::PLAY_IPSPEAKER_SOUND,
                    outStr,
                    0.0,
                    0.0,
                    0.0, 
                    0.0,
                    0.0, 
                    0.0,
                    0.0, 
                    0,   
                    std::map<std::string, std::string>{},
                    "",
                    0,
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    "",
                    ""
                );
            }

            std::pair<std::string, std::uint32_t> DeviceAgent::GetExternalSystemCodes(std::string eventName)
            {
                if (eventName.empty())
                    return std::pair<std::string, std::uint32_t>(); //error

                std::pair<std::string, std::uint32_t> p;
                std::string ledCode = "";
                std::uint32_t speakerAudioFileNum = 0;

                if (str_tolower(eventName) == "stealing") {
                    ledCode = "200113"; //WHITE/BLUE ON + RED ON (blinking) + BUZZER ON (fast bip sound) 
                    speakerAudioFileNum = 1; // first audio file
                }
                else if (str_tolower(eventName) == "falling") {
                    ledCode = "020111"; // WHITE/BLUE ON + AMBER ON (blinking) + BUZZER ON (continuos sound) 
                    speakerAudioFileNum = 2; // second audio file
                }
                else if (str_tolower(eventName) == "walking") {
                    ledCode = "002110"; //WHITE/BLUE ON + GREEN ON (blinking) + BUZZER OFF
                    speakerAudioFileNum = 3; // third audio file
                }
                else {
                    ledCode = "001110"; //WHITE/BLUE ON + GREEN ON + BUZZER OFF
                    speakerAudioFileNum = 4; // fourth audio file
                }

                p.first = ledCode;
                p.second = speakerAudioFileNum;

                return p;
            }

            std::string DeviceAgent::ExtractpolygonJSON(std::string str)
            {
                // Figure is available: {"figure":{"color":"#e040fb","points":[[0.8273073022312374,0.2235294117647059],[0.8799695740365112,0.2411764705882353],[0.902053752535497,0.9970588235294118],[0.36353955375253555,1]]},"label":"","showOnCamera":true}
                // Figure is null: {"figure":null,"label":"","showOnCamera":false}
                // Figure is empty: 
                std::size_t found, point, label, len_roi;
                std::string polygon, temp, point_xtract;

                //Figure is empty.
                if (str.size() < 20) {
                    //return "1$1$9999$1$9999$9999$1$9999";
                    return "";
                }

                found = str.find("null");

                //Figure is available
                if (found == std::string::npos)
                {
                    polygon = "";
                    point = str.find("points");
                    label = str.find("label");
                    len_roi = label - 4 - (point + 9);
                    temp = str.substr(point + 9, len_roi);

                    std::string start_k = "[";
                    std::string stop_k = "]";
                    std::string comma_k = ",";

                    std::size_t start_pos, stop_pos, comma_pos;
                    start_pos = temp.find(start_k);

                    while (start_pos != std::string::npos)
                    {
                        std::string sub_temp = temp.substr(start_pos, temp.length() - 1);

                        comma_pos = sub_temp.find(comma_k);
                        stop_pos = sub_temp.find(stop_k);

                        std::size_t len_x = comma_pos;
                        std::size_t len_y = stop_pos - (comma_pos);

                        std::string x = sub_temp.substr(1, len_x - 1);
                        std::string y = sub_temp.substr(comma_pos + 1, len_y - 1);

                        int x_core = int(std::stod(x) * 10000.0);
                        int y_core = int(std::stod(y) * 10000.0);

                        //ajust coordinates values to avoid them from being to close to the camera image's borders
                        if (x_core == 0)
                            x_core = 1;
                        else if (x_core == 10000)
                            x_core = 9999;

                        if (y_core == 0)
                            y_core = 1;
                        else if (y_core == 10000)
                            y_core = 9999;

                        std::string x_y = "$" + std::to_string(x_core) + "$" + std::to_string(y_core);
                        polygon += x_y;
                        start_pos = temp.find(start_k, start_pos + start_k.size());
                    }
                    polygon = polygon.substr(1, polygon.length() - 1);
                }
                else
                {
                    //Figure is null
                    //polygon = "1$1$9999$1$9999$9999$1$9999";
                    polygon = "";
                }
                return polygon;
            }

            std::vector<std::string> DeviceAgent::GetMultipleRois(std::string roiName, bool activationStatus)
            {
                std::vector<std::string> finalRois;
                //std::stringstream rois;
                roiName.erase(remove(roiName.begin(), roiName.end(), '#'), roiName.end()); // remove character "#" from roiName
                std::string _roiName = roiName + "1"; // for displaying full roi

                for (int index = 1; index <= m_deviceAgentSettings.maxRois; index++) {
                    std::string roiValue = ExtractpolygonJSON(settingValue(roiName + std::to_string(index)));
                    if (roiValue != "") {
                        //rois << roiValue + ";";
                        finalRois.push_back(roiValue);
                    }
                }
                return finalRois;
            }

            void DeviceAgent::getPluginSideSettings(Result<const ISettingsResponse*>* outResult) const
            {
                auto settingsResponse = new SettingsResponse();
                *outResult = settingsResponse;
            }

            Result<const ISettingsResponse*> DeviceAgent::settingsReceived()
            {
                //reset state of some controls
                auto settingsResponse = new SettingsResponse();

                auto assignNumericSetting =
                    [this](
                        const std::string& parameterName,
                        auto target,
                        std::function<void()> onChange = nullptr)
                {
                    using namespace nx::kit::utils;
                    using SettingType =
                        std::conditional_t<
                        std::is_floating_point<decltype(target->load())>::value, float, int>;

                    SettingType result{};
                    const auto parameterValueString = settingValue(parameterName);
                    if (fromString(parameterValueString, &result))
                    {
                        using AtomicValueType = decltype(target->load());
                        if (target->load() != AtomicValueType{ result })
                        {
                            target->store(AtomicValueType{ result });
                            if (onChange)
                                onChange();
                        }
                    }
                    else
                    {
                        NX_PRINT << "Received an incorrect setting value for '"
                            << parameterName << "': "
                            << nx::kit::utils::toString(parameterValueString)
                            << ". Expected an integer.";
                    }
                };

                // Get max ROI    
                m_deviceAgentSettings.maxRois = std::stoi(m_devAgent->kMaxRoisSetting);
                m_deviceAgentSettings.SRois = GetMultipleRois(m_devAgent->kClipRoiname, true);

                // Update ROIs
                m_ActionDetector->setROIs(m_deviceAgentSettings.SRois);

                // Recording
                if (settingValue("video.record.checkbox") == "true")
                {
                    // VideoWriterSettings
                    auto videoName = std::string(cameraName) + ".mp4";
                    auto videoSaveDir = fs::path(RecordingDir) / fs::path(saveFolder) / fs::path(cameraName);

                    if (!fs::exists(videoSaveDir))
                    {
                        fs::create_directories(videoSaveDir);
                    }

                    auto saveName = videoSaveDir / fs::path(videoName);
                    videoWriter = cv::VideoWriter(saveName.string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                        saveVideoFPS, saveVideoSize, true);

                    m_deviceAgentSettings.saveVideo = true;
                }
                else
                {
                    if (m_deviceAgentSettings.saveVideo)
                    {
                        videoWriter.release();
                    }
                    m_deviceAgentSettings.saveVideo = false;
                }    

                //get action name from the ui
                std::string actionName = settingValue("action.addremove.textfield");
                if (!actionName.empty()) { 
                    //add new action to the DB
                    if (settingValue("action.add.switchbutton") == "true")
                        AddAction(actionName);

                    //remove existing action from the DB
                    if (settingValue("action.remove.switchbutton") == "true")
                        RemoveAction(actionName);
                }   

                //display current action list on the notifications tab
                if (settingValue("action.showlist.switchbutton") == "true")
                    ShowActionList();

                //reset ui elements to default state/value
                settingsResponse->setValue("action.addremove.textfield", "");
                settingsResponse->setValue("action.add.switchbutton", "false");
                settingsResponse->setValue("action.remove.switchbutton", "false");
                settingsResponse->setValue("action.showlist.switchbutton", "false");

                return settingsResponse; 
            }


        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx

