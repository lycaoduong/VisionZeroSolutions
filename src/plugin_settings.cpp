#include "plugin_settings.h"

namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            DeviceAgentModel::DeviceAgentModel(fs::path pluginHomeDir) :
                m_pluginHomeDir(pluginHomeDir)
            {
            }

            DeviceAgentModel::~DeviceAgentModel()
            {
            }

            rapidjson::Document DeviceAgentModel::CoreUISettingsManifest()
            {
                rapidjson::Document coreUISettingsManifest(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc = coreUISettingsManifest.GetAllocator();

                // Seperate Core by GroupBox
                coreUISettingsManifest.AddMember("type", rapidjson::Value("GroupBox", alloc), alloc);
                coreUISettingsManifest.AddMember("caption", rapidjson::Value("Settings", alloc), alloc);

                // Add Core to UI
                rapidjson::Document m_CLIPCoreSettings = CLIPCoreSettings();

                // Make Items
                rapidjson::Document items(rapidjson::kArrayType);
                items.PushBack(rapidjson::Value(m_CLIPCoreSettings, alloc), alloc);

                coreUISettingsManifest.AddMember("items", items, alloc);

                return coreUISettingsManifest;
            }

            rapidjson::Document DeviceAgentModel::ActionUISettingsManifest()
            {
                rapidjson::Document actionUISettingsManifest(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc = actionUISettingsManifest.GetAllocator();

                actionUISettingsManifest.AddMember("type", rapidjson::Value("GroupBox", alloc), alloc);
                actionUISettingsManifest.AddMember("caption", rapidjson::Value("Actions Settings", alloc), alloc);
                
                rapidjson::Document items(rapidjson::kArrayType);
                items.PushBack(rapidjson::Value(ActionSettings(), alloc), alloc);

                actionUISettingsManifest.AddMember("items", items, alloc);
                return actionUISettingsManifest;
            }

            std::string DeviceAgentModel::GetDeviceAgentSettings()
            {
                //main object
                rapidjson::Document settingsModel(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc = settingsModel.GetAllocator();

                //rapidjson::Document licenseInfoManifest = LicenseInfoManifest(isLicensed, registrationID);
                rapidjson::Document m_CoreUISettingsManifest = CoreUISettingsManifest();
                rapidjson::Document m_ActionSettingsManifest = ActionUISettingsManifest();

                // Default from NX
                settingsModel.AddMember("type", rapidjson::Value("Settings", alloc), alloc);

                // Add UI setting to items
                rapidjson::Document items(rapidjson::kArrayType);
                //items.PushBack(rapidjson::Value(licenseInfoManifest, alloc), alloc);
                items.PushBack(rapidjson::Value(m_CoreUISettingsManifest, alloc), alloc);
                items.PushBack(rapidjson::Value(m_ActionSettingsManifest, alloc), alloc);

                // Add items to main json object
                settingsModel.AddMember("items", items, alloc);
                return GetJsonText(settingsModel);
            }

            std::string DeviceAgentModel::GetJsonText(const rapidjson::Document& doc)
            {
                std::string ret = "";
                if (!doc.IsObject())
                    return ret;

                rapidjson::StringBuffer buff;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writter(buff);
                doc.Accept(writter);
                return buff.GetString();
            }

            // ROI settings
            rapidjson::Document DeviceAgentModel::RoiObject(std::string roiName)
            {
                rapidjson::Document roiManifest(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc = roiManifest.GetAllocator();

                roiManifest.AddMember("type", rapidjson::Value("Repeater", alloc), alloc);
                roiManifest.AddMember("count", rapidjson::Value(kMaxRoisSetting.c_str(), alloc), alloc);

                rapidjson::Document templt(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc1 = templt.GetAllocator();
                templt.AddMember("type", rapidjson::Value("GroupBox", alloc1), alloc1);
                templt.AddMember("caption", rapidjson::Value("ROI #", alloc1), alloc1);

                rapidjson::Document checkItems(rapidjson::kArrayType);
                checkItems.PushBack(rapidjson::Value(roiName.c_str(), alloc), alloc);
                templt.AddMember("filledCheckItems", checkItems, alloc1);

                rapidjson::Document items(rapidjson::kArrayType);

                rapidjson::Document polygon(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc2 = polygon.GetAllocator();
                polygon.AddMember("type", rapidjson::Value("PolygonFigure", alloc2), alloc2);
                polygon.AddMember("name", rapidjson::Value(roiName.c_str(), alloc2), alloc2);
                polygon.AddMember("minPoints", rapidjson::Value(4), alloc2);
                polygon.AddMember("maxPoints", rapidjson::Value(8), alloc2);
                items.PushBack(rapidjson::Value(polygon, alloc), alloc);

                templt.AddMember("items", items, alloc1);

                roiManifest.AddMember("template", rapidjson::Value(templt, alloc), alloc);

                return roiManifest;
            }

            // Core settings
            rapidjson::Document DeviceAgentModel::CLIPCoreSettings()
            {
                rapidjson::Document vfdManifest(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc = vfdManifest.GetAllocator();

                vfdManifest.AddMember("type", rapidjson::Value("GroupBox", alloc), alloc);
                vfdManifest.AddMember("caption", rapidjson::Value("CLIP Settings", alloc), alloc);
                
                // Enable buttons
                rapidjson::Document enableRecordVideo(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc1 = enableRecordVideo.GetAllocator();
                enableRecordVideo.AddMember("type", rapidjson::Value("CheckBox", alloc1), alloc1);
                enableRecordVideo.AddMember("name", rapidjson::Value("video.record.checkbox", alloc1), alloc1);
                enableRecordVideo.AddMember("caption", rapidjson::Value("Enable Recording", alloc1), alloc1);
                enableRecordVideo.AddMember("description", rapidjson::Value("Enable Recording", alloc1), alloc1);
                enableRecordVideo.AddMember("defaultValue", rapidjson::Value(false), alloc1);
                enableRecordVideo.AddMember("enabled", rapidjson::Value(true), alloc1);

                // ROIs
                rapidjson::Document roiBox = RoiObject(kClipRoiname.c_str());

                rapidjson::Document items(rapidjson::kArrayType);
                // Merge Items
                //items.PushBack(rapidjson::Value(enableRecordVideo, alloc), alloc);

                items.PushBack(rapidjson::Value(roiBox, alloc), alloc);

                // Put all items to member
                vfdManifest.AddMember("items", items, alloc);

                return vfdManifest;
            }

            //action settings (add new action, remove existing action, show current action list)
            rapidjson::Document DeviceAgentModel::ActionSettings()
            {
                rapidjson::Document manifest(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc = manifest.GetAllocator();

                manifest.AddMember("type", rapidjson::Value("GroupBox", alloc), alloc);
                manifest.AddMember("caption", rapidjson::Value("Actions Settings", alloc), alloc);
                
                rapidjson::Document action(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc1 = action.GetAllocator();
                action.AddMember("type", rapidjson::Value("TextField", alloc1), alloc1);
                action.AddMember("name", rapidjson::Value("action.addremove.textfield", alloc1), alloc1);
                action.AddMember("caption", rapidjson::Value("Action Name"), alloc1);
                action.AddMember("description", rapidjson::Value("Please, type the action action. This can be a new action you wish to detect or an existing action you want to delete. (e.g: standing person)"), alloc1);
                action.AddMember("defaultValue", rapidjson::Value("", alloc1), alloc1);
                action.AddMember("enabled", rapidjson::Value(true), alloc1);  
               
                rapidjson::Document addButton(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc2 = addButton.GetAllocator();
                addButton.AddMember("type", rapidjson::Value("SwitchButton", alloc2), alloc2);
                addButton.AddMember("name", rapidjson::Value("action.add.switchbutton", alloc2), alloc2);
                addButton.AddMember("caption", rapidjson::Value("Add"), alloc2);
                addButton.AddMember("description", rapidjson::Value("Add new action you wish to detect."), alloc2);
                addButton.AddMember("defaultValue", rapidjson::Value(false), alloc2);
                addButton.AddMember("enabled", rapidjson::Value(true), alloc2); 

                rapidjson::Document removeButton(rapidjson::kObjectType);
                rapidjson::Document::AllocatorType& alloc3 = removeButton.GetAllocator();
                removeButton.AddMember("type", rapidjson::Value("SwitchButton", alloc3), alloc3);
                removeButton.AddMember("name", rapidjson::Value("action.remove.switchbutton", alloc3), alloc3);
                removeButton.AddMember("caption", rapidjson::Value("Remove"), alloc3);
                removeButton.AddMember("description", rapidjson::Value("Remove existing action. Please, note that the corresponding action will no longer be detected."), alloc3);
                removeButton.AddMember("defaultValue", rapidjson::Value(false), alloc3);
                removeButton.AddMember("enabled", rapidjson::Value(true), alloc3);

                rapidjson::Document showListButton(rapidjson::kObjectType); 
                rapidjson::Document::AllocatorType& alloc4 = showListButton.GetAllocator();
                showListButton.AddMember("type", rapidjson::Value("SwitchButton", alloc4), alloc4);
                showListButton.AddMember("name", rapidjson::Value("action.showlist.switchbutton", alloc4), alloc4);
                showListButton.AddMember("caption", rapidjson::Value("Display List"), alloc4);
                showListButton.AddMember("description", rapidjson::Value("Display current action list on the notification tab."), alloc4);
                showListButton.AddMember("defaultValue", rapidjson::Value(false), alloc4);
                showListButton.AddMember("enabled", rapidjson::Value(true), alloc4);
                
                rapidjson::Document items(rapidjson::kArrayType);
                items.PushBack(rapidjson::Value(action, alloc), alloc);
                items.PushBack(rapidjson::Value(addButton, alloc), alloc);
                //items.PushBack(rapidjson::Value(removeButton, alloc), alloc);
                items.PushBack(rapidjson::Value(showListButton, alloc), alloc);

                manifest.AddMember("items", items, alloc);
                return manifest;
            }

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx
