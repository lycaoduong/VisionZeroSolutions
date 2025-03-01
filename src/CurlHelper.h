#pragma once

#include "Define.h"
#include "Timer.h"
#include "curl/curl.h"
#include "utils.h"

#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <utility> 
#include <functional>
#include <iostream>

#pragma warning(disable:4251)

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

	struct HttpMemory {
		char* response;
		size_t size;
	};

	class NX_PLUGIN_CLIP CCurlHelper final
	{
	public:
		enum IntervalUnit
		{
			MICROSECONDS = 0,
			MILLISECONDS,
			SECONDS,
			MINUTES,
			HOURS,
		};

		enum HttpRequestType
		{
			HTTP_GET = 0,
			HTTP_POST
			//HTTP_PUT, /*currently not supported*/
			//HTTP_PATCH, /*currently not supported*/
			//HTTP_DELETE /*currently not supported*/
		};

		struct UrlInfo
		{
			std::string addr;
			std::string id;
			std::string password;
			int volume = 0;
			int location = 0;
			int repeat = 1;

			UrlInfo(std::string addr, std::string id, std::string password, int volume, int location, int repeat)
			{
				this->addr = std::move(addr);
				this->id = std::move(id);
				this->password = std::move(password);
				this->volume = volume;
				this->location = location;
				this->repeat = repeat;
			}
		};

		struct UrlInfo2
		{
			std::string addr;
			int port = 0;
			std::string resourcePath;
			std::string ledCode;

			UrlInfo2(std::string addr, int port, std::string resourcePath, std::string ledCode)
			{
				this->addr = std::move(addr);
				this->port = port;
				this->resourcePath = std::move(resourcePath);
				this->ledCode = std::move(ledCode);
			}
		};

		struct UrlInfo3
		{
			HttpRequestType type;
			std::string ipAddress;
			int port;
			std::string id;
			std::string password;
			std::string path;
			std::string parameters;
			std::string postfield; /*for HTTP POST request only*/
			int connectionTimeSec;
			int responseTimeSec;

			UrlInfo3(HttpRequestType httpRequestType, std::string nxServerIpAddress, int nxServerPort,
				std::string nxServerLoginID, std::string nxServerLoginPassword, std::string urlPath, std::string urlParameters,
				std::string postfield, /*for HTTP POST request only*/ int connectionTimeSec, int responseTimeSec)
			{
				this->type = httpRequestType;
				this->ipAddress = std::move(nxServerIpAddress);
				this->port = nxServerPort;
				this->id = std::move(nxServerLoginID);
				this->password = std::move(nxServerLoginPassword);
				this->path = std::move(urlPath);
				this->parameters = std::move(urlParameters);
				this->postfield = std::move(postfield);
				this->connectionTimeSec = connectionTimeSec;
				this->responseTimeSec = responseTimeSec;
			}
		};
	
		CCurlHelper();
		~CCurlHelper();

		CCurlHelper(const CCurlHelper& other) = delete;
		CCurlHelper& operator=(const CCurlHelper& rhs) = delete;

		//play sound on the IP speaker
		std::pair<bool, std::string> PlaySpeakerSound(std::vector<UrlInfo> urls, int interval, IntervalUnit unit = IntervalUnit::MILLISECONDS);
		std::pair<bool, std::string> CheckConnection(std::string ipAddress, long timeout);
		
		//control/set the Signal Tower LED
		std::pair<bool, std::string> ControlSignalTower(std::vector<UrlInfo2> urls, int interval, IntervalUnit unit = IntervalUnit::MILLISECONDS);

		// get embedding values from HTTP CLIP server
		std::string SendRequestToClipHttpServer(CCurlHelper::UrlInfo3 url, std::string& httpResponse); 

		// get event image description from Hugging Face 
		std::string SendHttpRequestToHuggingFace(CCurlHelper::UrlInfo3 req, std::string& httpResponse); 

	private:
		CURL** mHandles;
		CURLM* mMultiHandle;
		CTimer mTimer;
		mutable std::mutex mMutex;
	};


} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx
