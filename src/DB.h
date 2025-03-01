#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <sstream>

#ifdef _WIN32
#include <mysql.h>
#endif

#include "RestAPI.h"

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

    class DataBase
    {

    public:
	    DataBase(
            std::string hostname, 
            std::string userID, 
            std::string userPwd, 
            std::string dbName, 
            std::uint32_t port);

        ~DataBase();

        bool InsertRowDB(std::string actionName, std::string& errorMsg);
        bool DeleteRowDB(std::string actionName, std::string& errorMsg);
        std::pair<std::vector<std::string>, std::vector<float>> ReadRowsDB(std::string& errorMsg);

    private:
        bool Connect();
        void Disconnect();

        void ConvertStringToFloatList(std::string inputStr, char delimiter, std::vector<float>& fvalues);
        void GetEmbeddings(std::string actionName, std::string& outValuesStr);
        void ReadINIFile();      

    private:
        std::string m_hostname;
        std::string m_userID;
        std::string m_userPwd;
        std::string m_dbName;
        std::uint32_t m_port;

        MYSQL* conn = NULL;
        RestAPI restThread;
        std::string m_pluginHomeDir = "C:/Program Files/Network Optix/Nx Witness/MediaServer/plugins/nx_plugin_clip";
        std::string m_iniFilename = "C:/Program Files/Network Optix/Nx Witness/MediaServer/plugins/nx_plugin_clip/nx_plugin_clip.ini";

        std::string clipHostname;
        std::uint32_t clipPort;
        std::string clipUserID;
        std::string clipUserPassword;

        std::string hfHostname;
        std::string hfApiKey;
        std::string hfResourcePath;
    };

} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx