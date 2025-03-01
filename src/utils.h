// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once
#define _XOPEN_SOURCE

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#ifdef HAS_CPP17 // Use fs::
#include <filesystem>
namespace fs = std::filesystem;
#else //HAS_CPP14
#include <experimental/filesystem>
//namespace std {
//    namespace filesystem = experimental::filesystem;
//}
namespace fs = std::experimental::filesystem;
#endif 

#include <regex>
#include <time.h>
#ifdef _WIN32
#include <Windows.h>
#include <sysinfoapi.h>
#include <array>
#include <iomanip>
#include <conio.h>
#include <intrin.h>
#include <WinSock.h>
#include <IPHlpApi.h>
#pragma comment(lib, "iphlpapi.lib" )
#else
#include <cpuid.h>
#endif

#include <chrono>
#include <algorithm>
#include <cctype>
#include <stdio.h>
#include <stdlib.h>

#include <nx/sdk/helpers/uuid_helper.h>

#include "detection.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"

#include <opencv2/opencv.hpp>

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {
	
	void WriteLog(std::string filename, std::string word);

	const std::string GetCurrentDatetime();
	std::string TimestampToDatetime(int64_t timestampUs);
	int64_t DatetimeToTimestamp(std::vector<int> datetime);
	std::vector<int> DatetimeToListOfIntegers(std::string datetime);

	std::string ReadINI(std::string section, std::string key, std::string iniFilename);
	void WriteINI(std::string section, std::string key, std::string value, std::string iniFilename);

	std::string str_tolower(std::string s);
	int Find_ID(std::vector<std::string> arr, std::string k);
	void ReplaceAll(std::string& input, const std::string& from, const std::string& to);
	std::string CombineStrings(std::vector <std::string>& strList);
	std::vector<std::string> Split(std::string inputStr, char delimiter);
	
	std::string ParseHuggingFaceEventID(std::string inputJson, std::string& outStr);
	std::string ParseEmbeddedTextualData(std::string inputJson, std::string actionName, std::string& outStr);
	std::string ParseImageDescription(std::string inputJson, std::string actionName, std::string& outStr);

} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx

