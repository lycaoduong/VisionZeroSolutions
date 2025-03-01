#include "RestAPI.h"

#ifdef _WIN32
#include <mysql.h>
#endif

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

	void RestAPI::Run(
		std::string pluginHomePath,
		std::string pluginName,
		std::string deviceUuid,
		std::string deviceName,
		std::string ipAddress,
		int port,
		std::string id,
		std::string password,
		std::string startTime,
		std::string stopTime,
		std::vector<std::string> selectedEventTypes, 
		PluginOperationType operationType,
		std::string& outStr,
		double xTargetObjectPos,
		double yTargetObjectPos,
		double zTargetObjectPos,
		double xDeviceHomePos,
		double yDeviceHomePos,
		double zDeviceHomePos,
		double deviceSpeed,
		int goHomeTimeout,
		std::map<std::string, std::string> updatedLicenseList,
		std::string ftpIpAddress,
		int ftpPort,
		std::string ftpUserID,
		std::string ftpUserPassword,
		std::string ftpDownloadFilename,
		std::string ftpDownloadFolder,
		std::string localEventImageFilename,
		std::string ftpUploadEventImageFilename,
		std::string localOriginalImageFilename,
		std::string ftpUploadOriginalImageFilename)
	{
		if (pluginHomePath.empty()) {
			std::string plugin_name(PROJECT_NAME);
#ifdef HAS_CPP17 
			fs::path plugin_dir = fs::current_path() / fs::path("plugins") / fs::path(plugin_name);
#else
			std::experimental::filesystem::path plugin_dir = std::experimental::filesystem::current_path() / std::experimental::filesystem::path("plugins") / std::experimental::filesystem::path(plugin_name);
#endif
			m_iniFilename = plugin_dir.string() + "/nx_plugin_clip.ini";
			m_logFilename = plugin_dir.string() + "/LOGS/PLUGIN/COMM/comm.log";
		}
		else {
			m_iniFilename = pluginHomePath + "/nx_plugin_clip.ini";
			m_logFilename = pluginHomePath + "/LOGS/PLUGIN/COMM/comm.log";
		}

		m_pluginName = std::move(pluginName);
		m_deviceUuid = std::move(deviceUuid);
		m_deviceName = std::move(deviceName);
		m_ipAddress = std::move(ipAddress);
		m_port = port;
		m_id = std::move(id);
		m_password = std::move(password);
		m_startTime = std::move(startTime);
		m_stopTime = std::move(stopTime);
		m_selectedEventTypes = std::move(selectedEventTypes);
		m_operationType = operationType;
		m_xTargetOjectPos = xTargetObjectPos;
		m_yTargetOjectPos = yTargetObjectPos;
		m_zTargetOjectPos = zTargetObjectPos;
		m_xDeviceHomePos = xDeviceHomePos;
		m_yDeviceHomePos = yDeviceHomePos;
		m_zDeviceHomePos = zDeviceHomePos;
		m_deviceSpeed = deviceSpeed;
		m_goHomeTimeout = goHomeTimeout;
		m_updatedLicenseList = std::move(updatedLicenseList);

		m_ftpIpAddress = std::move(ftpIpAddress);
		m_ftpPort = ftpPort;
		m_ftpUserID = std::move(ftpUserID);
		m_ftpUserPassword = std::move(ftpUserPassword);
		m_ftpDownloadFilename = std::move(ftpDownloadFilename);
		m_ftpDownloadFolder = std::move(ftpDownloadFolder);
		m_localEventImageFilename = std::move(localEventImageFilename);
		m_ftpUploadEventImageFilename = std::move(ftpUploadEventImageFilename);
		m_localOriginalImageFilename = std::move(localOriginalImageFilename);
		m_ftpUploadOriginalImageFilename = std::move(ftpUploadOriginalImageFilename);		

		//Create a new thread and run the threadFunc method on it
		m_thread = std::thread(&RestAPI::RunOperation, this, std::ref(outStr));

		//Detach the thread so it runs independently of the main thread
		if (operationType == PluginOperationType::GET_EMBEDDED_TEXTUALDATA)
			m_thread.join();
		else
			m_thread.detach();
	}

	void RestAPI::RunOperation(std::string& outStr)
	{
		switch (m_operationType)
		{
		case PluginOperationType::GET_EMBEDDED_TEXTUALDATA:
			GetEmbeddings(outStr);
			break;
		case PluginOperationType::CONTROL_SIGNAL_TOWER:
			ControlSignalTower();
			break;
		case PluginOperationType::PLAY_IPSPEAKER_SOUND:
			PlaySpeakerSound();
			break;
		default:
			break;
		}
	}

	void RestAPI::GetEmbeddings(std::string& embeddings)
	{
		std::string actionOrDescription = m_pluginName;		
		std::string jsonPostData = "{\"text\":\"" + actionOrDescription + "\"}";
		ReplaceAll(jsonPostData, "\n", "");

		//CLIP http server currently only accepts "localhost" as ip address
		m_ipAddress = "localhost";

		CCurlHelper::UrlInfo3 url{
			CCurlHelper::HttpRequestType::HTTP_POST, /*HTTP method*/ 
			m_ipAddress, /*CLIP SERVER IP address*/
			m_port,/*CLIP SERVER port*/
			"", /* CLIP SERVER User Login ID : not required*/
			"", /* CLIP SERVER User Login Password : not required*/
			"/getTextualEmbedding", /*resource path*/
			"", /*additional resource parameters*/
			jsonPostData, /*POST field*/
			10, /*http connection timeout in seconds*/
			10 /*http response timeout in seconds*/
		};

		std::stringstream inputInfo;
		inputInfo << "http://" + url.ipAddress + ":" + std::to_string(url.port) + url.path + url.parameters << "\n";
		inputInfo << url.id << ":" << url.password << "\n";
		inputInfo << url.postfield << "\n";
		inputInfo << url.connectionTimeSec << "\n";
		inputInfo << url.responseTimeSec << "\n";

		std::string jsonHttpResponse;
		std::string httpTransferError = m_helper.SendRequestToClipHttpServer(url, jsonHttpResponse);
		if (!httpTransferError.empty()) 
			WriteLog(logFilename, "RestAPI::GetEmbeddings(): failed to received response from the CLIP SERVER. \n>> HTTP REQUEST INFO : \n" + inputInfo.str() + "\n>> HTTP TRANSFER ERROR: \n" + httpTransferError);

		std::string parseError = ParseEmbeddedTextualData(jsonHttpResponse, actionOrDescription, embeddings);
		if (!parseError.empty())
			WriteLog(logFilename, "RestAPI::GetEmbeddings(): parsing error has occurred. \n>> INPUT JSON : \n" + jsonHttpResponse + "\n>> PARSING ERROR: \n" + parseError);		
	}

	void RestAPI::ControlSignalTower()
	{
		std::string deviceName = m_deviceName;
		std::string ipAddress = m_ipAddress;
		std::string resourcePath = m_deviceUuid;
		int port = m_port;
		//std::string id = m_id;
		//std::string password = m_password;
		std::string eventName = m_startTime;
		std::string ledCode = m_stopTime;

		std::vector<CCurlHelper::UrlInfo2> urls;
		urls.emplace_back(ipAddress, port, resourcePath, ledCode);
		std::pair<bool, std::string> p = m_helper.ControlSignalTower(urls, 2000 /* timeout in milliseconds */);
		if (!p.first) 
			WriteLog(logFilename, "RestAPI::ControlSignalTower(): failed to change Signal Tower LED/BUZZER status to: " + ledCode + " (action name = " + eventName  + ", device name = " + deviceName + "). Error Info : " + p.second);
		else 
			WriteLog(logFilename, "RestAPI::ControlSignalTower(): changed Signal Tower LED/BUZZER status to: " + ledCode + " (action name = " + eventName + ", device name = " + deviceName + ")");

	}

	void RestAPI::PlaySpeakerSound()
	{
		std::string deviceName = m_deviceUuid;
		std::string ipAddress = m_ipAddress;
		//int port = m_port;
		std::string id = m_id;
		std::string password = m_password;
		std::string eventName = m_startTime;
		int location = std::atoi(m_stopTime.c_str());

		std::vector<CCurlHelper::UrlInfo> urls;
		urls.emplace_back(ipAddress, id, password, 50 /* volume: 0~100 */, location /* audio file location: 1~50 */, 1 /* repeat */);
		std::pair<bool, std::string> p = m_helper.PlaySpeakerSound(urls, 5000 /* timeout in milliseconds */);
		if (!p.first) 
			WriteLog(logFilename, "RestAPI::PlaySpeakerSound(): IP Speaker sound NOT PLAYED (audio file location = " + std::to_string(location) + ", action name = " + eventName + ", device name = " + deviceName + "). Error Info : " + p.second);
		else 
			WriteLog(logFilename, "RestAPI::PlaySpeakerSound(): IP Speaker sound PLAYED (audio file location = " + std::to_string(location) + ", action name = " + eventName + ", device name = " + deviceName + ")");
		
	}
	
} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx
