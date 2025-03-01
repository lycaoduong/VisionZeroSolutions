#pragma once

#include <string>
#include <filesystem>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"


namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {
            namespace fs = std::filesystem;

            class DeviceAgentModel
            {
            public:
                DeviceAgentModel(fs::path pluginHomeDir);
                ~DeviceAgentModel();


                struct UI_Settings
                {
                    bool add_CLIP_core_to_UI{ true };
                };
                UI_Settings uiSettings;

                // ROIs
                const std::string kClipRoiname{ "CLIP.ROI#" };


                rapidjson::Document CoreUISettingsManifest();
                rapidjson::Document ActionUISettingsManifest();


                std::string GetDeviceAgentSettings();

                std::string GetJsonText(const rapidjson::Document& doc);

                const std::string kMaxRoisSetting{ "5" }; // maximum number of ROIs per core type

            private:
                rapidjson::Document RoiObject(std::string roiName);
                rapidjson::Document CLIPCoreSettings();
                rapidjson::Document ActionSettings();

            private:
                fs::path m_pluginHomeDir;
            };

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx
