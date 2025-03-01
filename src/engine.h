// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/plugin.h>
#include <nx/sdk/analytics/helpers/engine.h>
#include <nx/sdk/analytics/i_uncompressed_video_frame.h>
#include <filesystem>


#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"

#include "modelLoader/include/modelLoader.h"
#include "core/include/coreUtils.h"


// UI
#include "plugin_settings.h"

//logging
#ifdef _WIN32
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h" 
#include "spdlog/async.h"
//DB
//#include <mysql.h>
#endif


namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            namespace fs = std::filesystem;

            class Engine: public nx::sdk::analytics::Engine
            {
            public:
                explicit Engine(fs::path pluginHomeDir);
                virtual ~Engine() override;

                //looging
                void WritePluginLog(const std::string logMessage);

                //DB
                //void ConnectDB();          
                //void DisconnectDB();
                //void InsertDB(std::string tableName, std::vector<std::string> values);
                //void DeleteDB(std::string tableName, std::vector<std::string> values);
                //std::vector<std::string> ReadDB(std::string tableName);


                //external systems
                std::string sigtowerIpAddress;
                std::int32_t sigtowerPort;
                std::string sigtowerResourcePath;

                std::string speakerIpAddress;
                std::int32_t spearkerPort;
                std::string spearkerUserID;
                std::string spearkerUserPasssword;

                bool connectToSpeaker;
                bool connectToSignalTower;

            protected:
                virtual std::string manifestString() const override;
                virtual void doObtainDeviceAgent(nx::sdk::Result<nx::sdk::analytics::IDeviceAgent*>* outResult, 
                    const nx::sdk::IDeviceInfo* deviceInfo) override;

            private:
                //logging
                std::vector<fs::path> CreateLogFolders();
                void InitLogger();
                std::string GetLogLevel(spdlog::level::level_enum lev);

                //ini file reading
                void ReadIniValues(std::string iniFilename);

            private:
                fs::path m_pluginHomeDir;
                DeviceAgentModel* m_devAgent;

                //logging
                const std::string m_pluginLoggerName = "plugin_daily_logger";
#ifdef _WIN32
                std::shared_ptr<spdlog::logger> m_pluginLogger;

                //DB
                //MYSQL* conn = NULL;
#endif
            };

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx

