#include "DB.h"


namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

	DataBase::DataBase(
		std::string hostname, 
		std::string userID, 
		std::string userPwd, 
		std::string dbName, 
		std::uint32_t port):
		m_hostname(std::move(hostname)),
		m_userID(std::move(userID)),
		m_userPwd(std::move(userPwd)),
		m_dbName(std::move(dbName)),
		m_port(port)
	{
		ReadINIFile();
		Connect();
	}

	DataBase::~DataBase()
	{
		Disconnect();
	}

	bool DataBase::Connect()
	{
		bool isConnected = false;
		conn = mysql_init(NULL);
		if (conn)
		{
			if (!mysql_real_connect(
				conn,
				m_hostname.c_str(),
				m_userID.c_str(),
				m_userPwd.c_str(),
				m_dbName.c_str(),
				m_port,
				NULL,
				0))
				isConnected = false;
			else
				isConnected = true;
		}
		return isConnected;
	}

	void DataBase::Disconnect()
	{
	#ifdef _WIN32
		if (conn) 
			mysql_close(conn);
	#endif
	}

	void DataBase::ConvertStringToFloatList(std::string inputStr, char delimiter, std::vector<float>& fvalues)
	{
		if (!inputStr.empty() || delimiter != '\0')
		{
			std::stringstream ss(inputStr);
			std::string word;
			while (std::getline(ss, word, delimiter)) {
				fvalues.push_back(std::stof(word));
			}
		}
	}

	void DataBase::GetEmbeddings(std::string actionName, std::string& outValuesStr)
	{
		if (actionName.empty())
			return;

		restThread.Run(
			m_pluginHomeDir, /* plugin's home directory*/
			actionName, /* action or description name*/
			"",
			"",
			clipHostname, /* CLIP HTTP Server's hostname */
			clipPort, /* CLIP HTTP Server's port number */
			"",/* CLIP HTTP Server's user ID: not used at the moment */
			"",/* CLIP HTTP Server's user Password: not used at the moment */
			"", 
			"",
			std::vector<std::string>(), 
			RestAPI::PluginOperationType::GET_EMBEDDED_TEXTUALDATA,
			outValuesStr, /* http response data */
			0.0,
			0.0, 
			0.0,
			0.0, 
			0.0,
			0.0, 
			0.0, 
			0,   
			std::map<std::string, std::string>{},
			"",
			0, 
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			""
		);
	}

	void DataBase::ReadINIFile()
	{
		clipHostname = ReadINI("CLIP_HTT_SERVER", "clip_ip_address", m_iniFilename);
		clipPort = std::atoi(ReadINI("CLIP_HTT_SERVER", "clip_port", m_iniFilename).c_str());
		clipUserID = ReadINI("CLIP_HTT_SERVER", "clip_user_id", m_iniFilename);
		clipUserPassword = ReadINI("CLIP_HTT_SERVER", "clip_user_password", m_iniFilename);

		hfHostname = ReadINI("HUGGING_FACE_SERVER", "access_url", m_iniFilename);
		hfApiKey = ReadINI("HUGGING_FACE_SERVER", "access_key", m_iniFilename);
		hfResourcePath = ReadINI("HUGGING_FACE_SERVER", "access_resource_path", m_iniFilename);
	}

	bool DataBase::InsertRowDB(std::string actionName, std::string& errorMsg)
	{
		if (actionName.empty()) {
			errorMsg = "invalid input parameters (action name is empty).";
			return false;
		}

		std::string outValuesStr = "";
		GetEmbeddings(actionName, outValuesStr);

		if (outValuesStr.empty()) {
			errorMsg = "something went wrong (ex: http server is disconnected).";
			return false;
		}

		// remove "[" character from the string
		outValuesStr.erase(remove(outValuesStr.begin(), outValuesStr.end(), '['), outValuesStr.end());
		outValuesStr.erase(remove(outValuesStr.begin(), outValuesStr.end(), ']'), outValuesStr.end());		
		ReplaceAll(outValuesStr, ",", "");

		std::ostringstream queryCmd;
		queryCmd << "INSERT INTO actions_values VALUES (";
		queryCmd << "'" << actionName << "', ";
		queryCmd << "'" << outValuesStr << "', ";
		queryCmd << "null, ";
		queryCmd << "null)";
	
	#ifdef _WIN32
		if (conn && !queryCmd.str().empty()) {
			if (mysql_query(conn, queryCmd.str().c_str())) {
				errorMsg = std::string(mysql_error(conn));
				return false;
			}
	
			//check for errors
			if (mysql_affected_rows(conn) <= 0) {
				errorMsg = "Action already exists in the DB.";
				return false;
			}
		}
	#endif

		return true;
	}

	bool DataBase::DeleteRowDB(std::string actionName, std::string& errorMsg)
	{
		if (actionName.empty()) {
			errorMsg = "invalid input parameters (action name is empty).";
			return false;
		}

		std::ostringstream queryCmd;
		queryCmd << "DELETE FROM actions_values WHERE action_name=";
		queryCmd << "'" << actionName << "'";

	#ifdef _WIN32
		if (conn && !queryCmd.str().empty()) {
			if (mysql_query(conn, queryCmd.str().c_str())) {
				errorMsg = std::string(mysql_error(conn));
				return false;
			}

			//check for errors
			if (mysql_affected_rows(conn) <= 0) {
				errorMsg = "Action does not exist in the DB.";
				return false;
			}
		}
	#endif

		return true;
	}

	std::pair<std::vector<std::string>, std::vector<float>> DataBase::ReadRowsDB(std::string& errorMsg)
	{
		std::pair<std::vector<std::string>, std::vector<float>> p;

		std::vector<std::string> names;
		std::vector<float> values;

		std::ostringstream queryCmd;
		queryCmd << "SELECT * FROM actions_values";

#ifdef _WIN32
		if (conn && !queryCmd.str().empty()) {
			if (mysql_query(conn, queryCmd.str().c_str())) {
				errorMsg = std::string(mysql_error(conn));
				return p;
			}

			MYSQL_RES* res = mysql_store_result(conn);
			std::uint64_t totalrows = mysql_num_rows(res);
			int numfields = mysql_num_fields(res);
			MYSQL_FIELD* mfield = NULL;
			MYSQL_ROW row;

			errorMsg = std::string(mysql_error(conn));

			while ((row = mysql_fetch_row(res))) {
				for (int idx = 0; idx < numfields; idx++) {
					if (idx == 0) {
						names.push_back(row[idx]); //action_name 
					}
					else if (idx == 1) { 
						ConvertStringToFloatList(row[idx], ' ', values); //action_values
					}
				}
			}
		}
#endif
		p.first = names;
		p.second = values;

		return p;
	}

} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx