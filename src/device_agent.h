// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <filesystem>
#include <nx/sdk/analytics/helpers/object_metadata_packet.h>
#include <nx/sdk/analytics/helpers/consuming_device_agent.h>
#include <nx/sdk/helpers/uuid_helper.h>
#include "engine.h"
#include "actions_detector.h"

#include "RestAPI2.h"


namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            class DeviceAgent: public nx::sdk::analytics::ConsumingDeviceAgent
            {
            public:
                using MetadataPacketList = std::vector<nx::sdk::Ptr<nx::sdk::analytics::IMetadataPacket>>;

            public:
                DeviceAgent(const nx::sdk::IDeviceInfo* deviceInfo,
                    fs::path pluginHomeDir,
                    Engine* engine);
                virtual ~DeviceAgent() override;

            protected:
                virtual std::string manifestString() const override;

                virtual bool pushUncompressedVideoFrame(
                    const nx::sdk::analytics::IUncompressedVideoFrame* videoFrame) override;

                virtual void doSetNeededMetadataTypes(
                    nx::sdk::Result<void>* outValue,
                    const nx::sdk::analytics::IMetadataTypes* neededMetadataTypes) override;

                // UI Settings
                virtual void getPluginSideSettings(nx::sdk::Result<const nx::sdk::ISettingsResponse*>* outResult) const override;
                virtual nx::sdk::Result<const nx::sdk::ISettingsResponse*> settingsReceived() override;

            private:

                /* Ly Cao Duong */
                nx::sdk::Ptr<nx::sdk::analytics::ObjectMetadataPacket> detectionsToObjectMetadataPacket(
                    const DetectionList& detections,
                    int64_t timestampUs,
                    const Frame& frame);

                MetadataPacketList eventsToEventMetadataPacketList(
                    const DetectionList& events, 
                    int64_t timestampUs, 
                    const Frame& frame);

                MetadataPacketList processFrame(
                    const nx::sdk::analytics::IUncompressedVideoFrame* videoFrame);

                // Model Loader
                bool loadModel(fs::path model_dir, OnnxLoader::Model& onnxModel);

                /* Luciano Roberto */
                void AddAction(std::string actionName);
                void RemoveAction(std::string actionName);
                void ShowActionList();

                //External systems
                void ControlSignalTower(std::string ledCode, std::string eventName);
                void PlaySpeakerSound(std::uint32_t audioFileNumber, std::string eventName);
                std::pair<std::string, std::uint32_t> GetExternalSystemCodes(std::string eventName);

            private:

                // Object types
                const std::string kPersonObjectType = "nx.lcd.vision.person";
                // Event types
                const std::string kPersonEventType = "nx.lcd.vision.newPerson";

                int detDurationUs = 0; //us 100ms: 100000


            private:
                bool m_terminated = false;
                bool m_terminatedPrevious = false;
                ActionDetector* m_ActionDetector;

                //real-time video storage
                cv::VideoWriter videoWriter;
                std::string RecordingDir = "D:/";
                std::string saveFolder = "LCDSolutions";
                cv::Size saveVideoSize = cv::Size(640, 480);
                std::string cameraName;
                int saveVideoFPS = 24;
                cv::Mat saveFrame;

                fs::path m_pluginHomeDir;
                std::string m_iniFilename;

                RestAPI m_restThread;
                Rest& rest = Rest::getInstance();
                std::thread procThread;

                int m_frameIndex = 0; /**< Used for generating the detection in the right place. */

                struct DeviceAgentSettings
                {
                    //std::atomic<int> assumedFPS{ 6 };
                    //std::atomic<int> assumedFPS{ 3 };
                    std::atomic<int> assumedFPS{ 5 };

                    std::atomic<int> trackBuffer{ 30 };

                    /*Camera FPS*/
                    std::atomic<int> camFPS{ 0 };
                    std::atomic<int64_t> time_us_from_camera{ 0 };

                    std::atomic<int> skipFrame{ 2 };

                    //maximum number of ROIs per core type
                    std::atomic<int> maxRois;
                    std::vector<std::string> SRois;
                    bool saveVideo = false;
                };
                DeviceAgentSettings m_deviceAgentSettings;

                DeviceAgentModel* m_devAgent;

                std::string ExtractpolygonJSON(std::string str);
                std::vector<std::string> GetMultipleRois(std::string roiName, bool activationStatus);

                Engine* m_engine;
            private:
                OnnxLoader::Model m_Detmodel;
                fs::path m_DetmodelName = fs::path("coco.onnx");

                OnnxLoader::Model m_ClipVisualmodel;
                fs::path m_ClipVisualmodelName = fs::path("clip_visualL.onnx");

                std::vector<std::string> trackingBuffer;
                int maxTrackingBuffer = 30;
            };

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx

