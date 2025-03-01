#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <string>
#include <queue>

#include <string>
#include <fstream>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CurlHelper.h"

#ifdef _WIN32
#include <mysql.h>
#endif

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

	struct RestMemory {
		char* response;
		size_t size;
	};

	enum RequestType
	{
		HTTP_GET = 0,
		HTTP_POST
		//HTTP_PUT, /*currently not supported*/
		//HTTP_PATCH, /*currently not supported*/
		//HTTP_DELETE /*currently not supported*/
	};

	struct RestUrlInfo
	{
		RequestType type;
		std::string ipAddress;
		int port;
		std::string id;
		std::string password;
		std::string path;
		std::string parameters;
		std::string postfield; /*for HTTP POST request only*/
		int connectionTimeSec;
		int responseTimeSec;

		RestUrlInfo(RequestType httpRequestType, std::string nxServerIpAddress, int nxServerPort,
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

	class Rest
	{
	public:
		Rest();

		~Rest();

		static Rest& getInstance();

		void Stop();

		void ProcessEventImage(std::pair<std::vector<std::string>, cv::Mat> p);
		void Init(std::string iniFilename);

	private:
		void Run();

		//Event image processing
		void SaveImageDescriptionEmbeddings(std::string imageUrl, std::string imagePath);
		std::string GetImageDescription(std::string imagePath, std::string imageUrl);
		void GetEmbeddings(std::string description, std::string& embeddings);
		bool SaveEmbeddingsToDB(std::string description, std::string embeddings, std::string imageUrl, std::string imagePath);

		//DB
		void ConnectDB(std::string m_dbHostname, std::uint32_t m_dbPort, std::string m_dbUserID, std::string  m_dbUserPwd, std::string m_dbName);
		bool IsConnected() { return isConnected; }
		void Disconnect();  

	private:
		std::thread mThread;

		std::condition_variable mCV;
		std::mutex mMutex;
		std::queue<std::pair<std::vector<std::string>, cv::Mat>> mQueue;
		bool mStop;

		CCurlHelper m_helper;

		//Local HTTP Server for saving event images
		std::string m_eventImageDir;
		std::string m_storageIpAddress;
		std::int32_t m_storagePort;
		std::string m_storageResourcePath;

		//HTTP CLIP Server
		std::string m_clipHostname;
		std::int32_t m_clipPort;
		std::string m_clipUserID;
		std::string m_clipUserPassword;

		//HTTP Hugging Face Server
		std::string m_hfHostname;
		std::string m_hfApiKey;
		std::string m_hfResourcePath;

		//DB
		MYSQL* conn = NULL;
		bool isConnected = false;

		std::string logFilename = "C:/Program Files/Network Optix/Nx Witness/MediaServer/plugins/nx_plugin_clip/LOGS/comm.log";
		std::string m_iniFilename;

		//std::uint64_t eventCnt = 1;
	};

} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx