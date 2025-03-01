// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "utils.h"
#ifndef _WIN32
#include <math.h>
#include "readIni.h"
#endif

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

    using namespace std::chrono;

    //log file writing
    /*void WriteLog(std::string filename, std::string word)
    {
        std::ofstream file;
        file.open(filename.c_str(), std::ofstream::app);
        if (file.is_open()) {
            file << word << "\n";
        }
        file.close();
    }*/

    void WriteLog(std::string filename, std::string word)
    {
        std::ofstream file;
        file.open(filename.c_str(), std::ofstream::app);
        if (file.is_open()) {
            file << GetCurrentDatetime() << " " << word << "\n";
        }
        file.close();
    }

    //Get current date and time
    const std::string GetCurrentDatetime()
    {
        time_t     now = time(0);
        tm  tstruct;
        char       buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tstruct); // format: YYYY-MM-DD HH:mm:ss
        return buf;
    }

    std::string TimestampToDatetime(int64_t timestampUs)
    {
        time_t datetime = time(0);

        tm  tstruct;
        char       buf[80];
        tstruct = *localtime(&datetime);
        strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tstruct); // format: YYYY-MM-DD HH:mm:ss

        return std::string(buf);
    }

    int64_t DatetimeToTimestamp(std::vector<int> datetime)
    {
        if (datetime.size() != 6)
            return 0;
        tm timeinfo;
        timeinfo.tm_year = datetime[0] - 1900;
        timeinfo.tm_mon = datetime[1] - 1;
        timeinfo.tm_mday = datetime[2];
        timeinfo.tm_hour = datetime[3];
        timeinfo.tm_min = datetime[4];
        timeinfo.tm_sec = datetime[5];
        return static_cast<int64_t>(std::mktime(&timeinfo)) * 1000000;
    }

    //input datetime format: 2023-05-22%2000%3A00%3A00
    std::vector<int> DatetimeToListOfIntegers(std::string datetime)
    {
        std::vector<int> datetimeParts;

        std::vector <std::string> list1 = Split(datetime, '-');
        datetimeParts.push_back(atoi(list1.at(0).c_str())); // year
        datetimeParts.push_back(atoi(list1.at(1).c_str())); // month
        ReplaceAll(list1.at(2), "%20", "-");
        ReplaceAll(list1.at(2), "%3A", ":");
        std::vector <std::string> list2 = Split(list1.at(2), '-');
        datetimeParts.push_back(atoi(list2.at(0).c_str())); // day
        std::vector <std::string> list3 = Split(list2.at(1), ':');
        datetimeParts.push_back(atoi(list3.at(0).c_str())); // hour
        datetimeParts.push_back(atoi(list3.at(1).c_str())); // minute
        datetimeParts.push_back(atoi(list3.at(2).c_str())); // second

        return datetimeParts;
    }

    //read value from INI file
    std::string ReadINI(std::string section, std::string key, std::string iniFilename)
    {
#ifdef _WIN32
        char szKeyValue[1024];
        GetPrivateProfileStringA(section.c_str(), key.c_str(), "", szKeyValue, sizeof(szKeyValue), iniFilename.c_str());
        return std::string(szKeyValue);

#else
        char buffIni[1024];
        ini_gets(section.c_str(), key.c_str(), "", buffIni, 1024, iniFilename.c_str());
        return std::string(buffIni);
#endif
    }

    //write value to INI file
    void WriteINI(std::string section, std::string key, std::string value, std::string iniFilename)
    {
#ifdef _WIN32
        WritePrivateProfileStringA(section.c_str(), key.c_str(), value.c_str(), iniFilename.c_str());
#endif
    }

    std::string str_tolower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) {
                return std::tolower(c);
            }
        );

        return s;
    }

    int Find_ID(std::vector<std::string> arr, std::string k)
    {
        std::vector<std::string>::iterator it;
        it = std::find(arr.begin(), arr.end(), k);
        if (it != arr.end())
            return (it - arr.begin());
        else
            return -1;
    }

    void ReplaceAll(std::string& input, const std::string& from, const std::string& to)
    {
        auto pos = 0;
        while (true)
        {
            size_t startPosition = input.find(from, pos);
            if (startPosition == std::string::npos)
                return;
            input.replace(startPosition, from.length(), to);
            pos += to.length();
        }
    }

    std::vector<std::string> Split(std::string inputStr, char delimiter)
    {
        if (inputStr.empty() || delimiter == '\0')
            return std::vector<std::string>();

        std::vector<std::string> words;
        std::stringstream ss(inputStr);
        std::string word;

        while (getline(ss, word, delimiter)) {
            words.push_back(word);
        }

        return words;
    }

    std::string ParseEmbeddedTextualData(std::string inputJson, std::string actionName, std::string& outStr)
    {
        std::string errorMsg = "";
        if (inputJson.empty())
            return "input JSON string is empty.";

        if (actionName.empty())
            return "action name is empty.";

        rapidjson::Document document;
        document.Parse(inputJson.c_str());
        if (document.HasParseError())
            return "Json parse error: " + std::to_string(document.GetParseError());

        if (!document.IsObject())
            return "root value is not an object: " + inputJson;
       
        if (document.HasMember("msg") && document["msg"].IsString())
            if( document["msg"].GetString() == "JSON decode error")
                return "json response contains decode error.";

        if(document.HasMember("embedding") && document["embedding"].IsString())
            outStr = document["embedding"].GetString();

        return errorMsg;
    }
    
    std::string ParseImageDescription(std::string inputJson, std::string actionName, std::string& imageURL)
    {
        std::string errorMsg = "";
        if (inputJson.empty())
            return "input JSON string is empty.";

        if (actionName.empty())
            return "action name is empty.";

        rapidjson::Document document;
        document.Parse(inputJson.c_str());
        if (document.HasParseError())
            return "Json parse error: " + std::to_string(document.GetParseError());

        if (!document.IsObject())
            return "root value is not an object: " + inputJson;

        if (document.HasMember("text") && document["text"].IsString() &&
            document.HasMember("embedding") && document["embedding"].IsString())
        {
            if (document["text"].GetString() == actionName)
                imageURL = document["embedding"].GetString();
        }

        return errorMsg;
    }    

    std::string CombineStrings(std::vector<std::string>& strList)
    {
        // remove "default" action. This is a system reserved action which is not meant to be controlled by users.
        if (std::count(strList.begin(), strList.end(), "default") > 0) {
            auto it = std::find(strList.begin(), strList.end(), "default");
            strList.erase(strList.begin() + (it - strList.begin()));
        }

        std::stringstream ss;
        std::size_t size = strList.size();
        for (int idx = 0; idx < size; idx++) {
            if (idx != (size - 1))
                ss << strList[idx] + ", ";
            else
                ss << strList[idx];
        }

        std::string str = ss.str();
        if (str.empty())
            str = "None";

        return str;
    }

    std::string ParseHuggingFaceEventID(std::string inputJson, std::string& outStr)
    {
        std::string errorMsg = "";
        if (inputJson.empty())
            return "input JSON string is empty.";

        rapidjson::Document document;
        document.Parse(inputJson.c_str());
        if (document.HasParseError())
            return "Json parse error: " + std::to_string(document.GetParseError());

        if (!document.IsObject())
            return "root value is not an object: " + inputJson;

        if (document.HasMember("event_id") && document["event_id"].IsString())
            outStr = document["event_id"].GetString();

        return errorMsg;
    }

} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx
