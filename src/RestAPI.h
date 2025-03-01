#include <thread>
#include <string>

#include "CurlHelper.h"
#include "utils.h"

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

	class RestAPI
	{
	public:
		enum PluginOperationType {			
			GET_EMBEDDED_TEXTUALDATA,
			CONTROL_SIGNAL_TOWER,
			PLAY_IPSPEAKER_SOUND
		};

	public:
		void Run(
			std::string m_pluginHomePath,
			std::string pluginName,
			std::string deviceUuid,
			std::string deviceName,
			std::string ipAddress,
			int port,
			std::string id,
			std::string password,
			std::string m_startTime,
			std::string m_stopTime,
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
			std::string ftpUploadOriginalImageFilename);
	
	private:
		void RunOperation(std::string& outStr);
		void GetEmbeddings(std::string& embeddings);
		void ControlSignalTower();
		void PlaySpeakerSound();

	private:
		std::string m_logFilename;
		std::string m_iniFilename;
		std::string m_pluginName;
		std::thread m_thread;
		CCurlHelper m_helper;

		std::string m_deviceUuid;
		std::string m_deviceName;
		std::string m_ipAddress;
		int m_port;
		std::string m_id;
		std::string m_password;
		std::string m_path;
		std::string m_parameters;
		int m_timeout;
		double m_xTargetOjectPos, m_yTargetOjectPos, m_zTargetOjectPos;
		double m_xDeviceHomePos, m_yDeviceHomePos, m_zDeviceHomePos;
		double m_deviceSpeed;
		int m_goHomeTimeout;

		std::string m_startTime;
		std::string m_stopTime;
		std::vector<std::string> m_selectedEventTypes;

		PluginOperationType m_operationType;
		std::string m_settingsModel;
		std::string m_pluginUuid;
		std::pair<std::string, std::string> m_modelStreamPair;

		std::map<std::string, std::string> m_updatedLicenseList;
		std::vector<std::string> m_activeCoreTypes;

		std::string m_ftpIpAddress;
		int m_ftpPort;
		std::string m_ftpUserID;
		std::string m_ftpUserPassword;
		std::string m_ftpDownloadFilename;
		std::string m_ftpDownloadFolder;
		std::string m_localEventImageFilename;
		std::string m_ftpUploadEventImageFilename;
		std::string m_localOriginalImageFilename;
		std::string m_ftpUploadOriginalImageFilename;

		std::string logFilename = "C:/Program Files/Network Optix/Nx Witness/MediaServer/plugins/nx_plugin_clip/LOGS/comm.log";
	};

} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx
