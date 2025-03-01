// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "engine.h"
#include "device_agent.h"
#include "utils.h"

namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            using namespace nx::sdk;
            using namespace nx::sdk::analytics;

            Engine::Engine(fs::path pluginHomeDir):
                // Call the DeviceAgent helper class constructor telling it to verbosely report to stderr.
                nx::sdk::analytics::Engine(/*enableOutput*/ true),
                m_pluginHomeDir(pluginHomeDir)
            {
                InitLogger();

                WritePluginLog("==========================================================================\n"
                    "Engine::Engine() : NX SERVER STARTED.\n"
                    "==========================================================================");

                m_devAgent = new DeviceAgentModel(m_pluginHomeDir.string());

                //ConnectDB();

                ReadIniValues(m_pluginHomeDir.string() + "/nx_plugin_clip.ini");
            }

            Engine::~Engine()
            {
                //DL_net->~Net();

                //DisconnectDB();

                //clean-up
                WritePluginLog("Engine::~Engine() : NX SERVER STOPPED.");
#ifdef _WIN32  
                spdlog::drop(m_pluginLoggerName);
#endif
            }

            /**
             * Called when the Server opens a video-connection to the camera if the plugin is enabled for this
             * camera.
             *
             * @param outResult The pointer to the structure which needs to be filled with the resulting value
             *     or the error information.
             * @param deviceInfo Contains various information about the related device such as its id, vendor,
             *     model, etc.
             */
            void Engine::doObtainDeviceAgent(Result<IDeviceAgent*>* outResult, const IDeviceInfo* deviceInfo)
            {
                *outResult = new DeviceAgent(deviceInfo, m_pluginHomeDir, this);
            }

            void Engine::ReadIniValues(std::string iniFilename)
            {
                sigtowerIpAddress = ReadINI("SIGNAL_TOWER_LED", "sigtower_ip_address", iniFilename);
                sigtowerPort = std::atoi(ReadINI("SIGNAL_TOWER_LED", "sigtower_port", iniFilename).c_str());
                sigtowerResourcePath = ReadINI("SIGNAL_TOWER_LED", "sigtower_resource_path", iniFilename);

                speakerIpAddress = ReadINI("IP_SPEAKER", "speaker_ip_address", iniFilename);
                spearkerPort = std::atoi(ReadINI("IP_SPEAKER", "spearker_port", iniFilename).c_str());
                spearkerUserID = ReadINI("IP_SPEAKER", "spearker_user_id", iniFilename);
                spearkerUserPasssword = ReadINI("IP_SPEAKER", "spearker_user_password", iniFilename);

                connectToSignalTower = ReadINI("EVENT_SETTINGS", "connect_to_signaltower", iniFilename) == "yes" ? true : false;
                connectToSpeaker = ReadINI("EVENT_SETTINGS", "connect_to_ipspeaker", iniFilename) == "yes" ? true : false;
            }

            /**
             * @return JSON with the particular structure. Note that it is possible to fill in the values
             *     that are not known at compile time, but should not depend on the Engine settings.
             */
            std::string Engine::manifestString() const
            {
                // Ask the Server to supply uncompressed video frames in YUV420 format (see
                // https://en.wikipedia.org/wiki/YUV).
                //
                // Note that this format is used internally by the Server, therefore requires minimum
                // resources for decoding, thus it is the recommended format.
                return /*suppress newline*/ 1 + (const char*) R"json(
                {
                    "capabilities": "needUncompressedVideoFrames_bgr",
                    "deviceAgentSettingsModel":)json" + m_devAgent->GetDeviceAgentSettings() + R"json(
                }
                )json";
            }

            // "deviceAgentSettingsModel":)json" + m_devAgent->GetDeviceAgentSettings() + R"json(

            //create folders where the log files will be stored, in case they do not exist.
            std::vector<fs::path> Engine::CreateLogFolders()
            {
                std::vector<fs::path> dirList;

                fs::path pluginLogDir(m_pluginHomeDir.string() + "/LOGS");

                if (!fs::is_directory(pluginLogDir)) {
                    fs::create_directories(pluginLogDir);
                    fs::permissions(pluginLogDir, fs::perms::all, fs::perm_options::add);
                }

                dirList.push_back(pluginLogDir);

                return dirList;
            }

            //initialize logger objects
            void Engine::InitLogger()
            {
                std::vector<fs::path> dirList = CreateLogFolders();
#ifdef _WIN32
                m_pluginLogger = spdlog::daily_logger_mt<spdlog::async_factory>(m_pluginLoggerName, spdlog::filename_t(dirList[0].string() + "/nx_plugin_clip.log"), 0, 0, false, 0);
                m_pluginLogger->set_pattern("%Y-%m-%d %H:%M:%S %v");
                m_pluginLogger->set_level(spdlog::level::info);
#endif
            }

            // get log level
            std::string Engine::GetLogLevel(spdlog::level::level_enum lev)
            {
#ifdef _WIN32
                std::string strLevel;
                switch (lev)
                {
                case spdlog::level::level_enum::trace:
                    strLevel = "TRACE";
                    break;
                case spdlog::level::level_enum::debug:
                    strLevel = "DEBUG";
                    break;
                case spdlog::level::level_enum::info:
                    strLevel = "INFO";
                    break;
                case spdlog::level::level_enum::warn:
                    strLevel = "WARN";
                    break;
                case spdlog::level::level_enum::err:
                    strLevel = "ERROR";
                    break;
                case spdlog::level::level_enum::critical:
                    strLevel = "CRITICAL";
                    break;
                case spdlog::level::level_enum::off:
                    strLevel = "OFF";
                    break;
                default: // invalid log level
                    strLevel = "";
                    break;
                }
                return strLevel;
#endif
            }
            
            //write plugin-related log messages to a file created on a daily basis
            void Engine::WritePluginLog(const std::string logMessage)
            {
#ifdef _WIN32
                auto logger = spdlog::get(m_pluginLoggerName);
                if (logger) {
                    logger->info("{0} {1}", GetLogLevel(logger->level()), logMessage.c_str());
                    logger->flush();
                }
#endif
            }
          
        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx
