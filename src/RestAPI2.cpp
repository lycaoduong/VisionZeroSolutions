#include "RestAPI2.h"

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

    Rest::Rest() : mThread{}, mCV{}, mMutex{}, mQueue{}, mStop{ false }
    {
        mThread = std::thread(&Rest::Run, this); //create thread and execute Run()
    }

    Rest::~Rest()
    {
        //Join thread
        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    Rest& Rest::getInstance()
    {
        static Rest rest;
        return rest;
    }

    void Rest::Stop()
    {
        {
            //Set the stop flag
            std::lock_guard<std::mutex> lk(mMutex);
            mStop = true;
        }
        mCV.notify_one();
    }

    void Rest::Init(std::string iniFilename)  
    {        
        m_iniFilename = iniFilename;

        //Local HTTP Server for saving event images
        m_eventImageDir = ReadINI("HTTP_IMAGE_STORAGE_SERVER", "http_local_storage_dir", iniFilename);
        m_storageIpAddress = ReadINI("HTTP_IMAGE_STORAGE_SERVER", "http_ip_address", iniFilename);
        m_storagePort = std::atoi(ReadINI("HTTP_IMAGE_STORAGE_SERVER", "http_port", iniFilename).c_str());
        m_storageResourcePath = ReadINI("HTTP_IMAGE_STORAGE_SERVER", "http_resource_path", iniFilename);

        //HTTP CLIP Server
        m_clipHostname = ReadINI("CLIP_HTT_SERVER", "clip_ip_address", iniFilename);
        m_clipPort = std::atoi(ReadINI("CLIP_HTT_SERVER", "clip_port", iniFilename).c_str());
        //m_clipUserID = ReadINI("CLIP_HTT_SERVER", "clip_user_id", iniFilename);
        //m_clipUserPassword = ReadINI("CLIP_HTT_SERVER", "clip_user_password", iniFilename);

        //HTTP Hugging Face Server
        m_hfHostname = ReadINI("HUGGING_FACE_SERVER", "access_url", iniFilename);
        m_hfApiKey = ReadINI("HUGGING_FACE_SERVER", "access_key", iniFilename);
        m_hfResourcePath = ReadINI("HUGGING_FACE_SERVER", "access_resource_path", iniFilename);

        std::string dbHostname = ReadINI("DB", "db_ip_address", iniFilename);
        std::uint32_t dbPort = std::atoi(ReadINI("DB", "db_port", iniFilename).c_str());
        std::string dbUserID = ReadINI("DB", "db_user_id", iniFilename);
        std::string dbUserPwd = ReadINI("DB", "db_user_password", iniFilename);
        std::string dbName = ReadINI("DB", "db_name", iniFilename);

        ConnectDB(dbHostname, dbPort, dbUserID, dbUserPwd, dbName);
    }

    void Rest::ConnectDB(std::string dbHostname, std::uint32_t dbPort, std::string dbUserID, std::string  dbUserPwd, std::string dbName)
    {
        conn = mysql_init(NULL);
        if (conn) {
            if (!mysql_real_connect(conn, dbHostname.c_str(), dbUserID.c_str(), dbUserPwd.c_str(), dbName.c_str(), dbPort, NULL, 0)) {
                Disconnect();
                isConnected = false;
                WriteLog(logFilename, "Rest::ConnectDB(): failed to connect to DB (" + dbHostname + ":" + std::to_string(dbPort) + "). Error info : " + std::string(mysql_error(conn)));
            }
            else {
                isConnected = true;
                WriteLog(logFilename, "Rest::ConnectDB(): connected to DB (" + dbHostname + ":" + std::to_string(dbPort) + ")");
            }
        }
    }

    void Rest::Disconnect()
    {
        if (conn) {
            mysql_close(conn);
            WriteLog(logFilename, "Rest::Disconnect(): DB disconnected.");
        }
    }

    void Rest::ProcessEventImage(std::pair<std::vector<std::string>, cv::Mat> p)
    {
        {
            //Push a request on the queue
            std::lock_guard<std::mutex> lk(mMutex);
            mQueue.push(p);
        }
        mCV.notify_one();
    }

    void Rest::Run()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(mMutex);

            while (mQueue.empty()) {
                // release lock as long as the wait and reaquire it afterwards.
                mCV.wait(lock);
            }

            //Pop the job off the front of the queue
            auto p = mQueue.front();

            std::string eventName = p.first[0];
            std::string deviceName = p.first[1];
            float bBox_x = std::stof(p.first[2]);
            float bBox_y = std::stof(p.first[3]);
            float bBox_width = std::stof(p.first[4]);
            float bBox_height = std::stof(p.first[5]);
            std::uint64_t timestampUs = std::strtoll(p.first[6].c_str(), NULL, 0);

            try {  
                //set the image path and url
                ReplaceAll(eventName, ",", "_");
                fs::path fileDir = fs::path(m_eventImageDir + "/" + deviceName);
                if (!fs::exists(fileDir))
                    fs::create_directories(fileDir);

                std::string imagePath = fileDir.string() + "/" + deviceName + "_" + TimestampToDatetime(timestampUs) + "_" + eventName + ".png";
                std::string imageUrl = "http://" + m_storageIpAddress + ":" + std::to_string(m_storagePort) + m_storageResourcePath + "/" + deviceName + "/" + deviceName + "_" + TimestampToDatetime(timestampUs) + "_" + eventName + ".png";

                //save the cropped image
                cv::Rect rect(
                    p.second.cols * bBox_x, 
                    p.second.rows * bBox_y, 
                    p.second.cols * bBox_width,
                    p.second.rows * bBox_height);              
                cv::imwrite(imagePath, p.second(rect));

                //get event image description and save its embeddings into the DB
                SaveImageDescriptionEmbeddings(imageUrl, imagePath);
            }
            catch (std::exception e) {}

            mQueue.pop();
        }
    }

    //save the event image description's embeddings to the DB
    void Rest::SaveImageDescriptionEmbeddings(std::string imageUrl, std::string imagePath)
    {
        // get the image's description from the Hugging Face Server
        std::string imageDescription = GetImageDescription(imageUrl, imagePath);
        //std::string imageDescription = "The person in the image appears to be a male, dressed in a bulky, olive-green coat and carrying a bag over his shoulder. He might be " + std::to_string(eventCnt) + " of age.";
        //eventCnt++;

        // convert string description into embedded vector (using the CLIP Server)
        std::string embeddedValues = "";
        GetEmbeddings(imageDescription, embeddedValues);
        if (embeddedValues.empty()) {
            WriteLog(logFilename, "Rest::SaveImageDescriptionEmbeddings() : retrived invalid/empty embeddings for the following image description : " + imageDescription + "\nPlease, make sure the HTTP CLIP Server is running. Additionally, you may want your settings in the INI file: " + m_iniFilename);
            return;
        }

        // save the embeddings vector into the DB
        SaveEmbeddingsToDB(imageDescription, embeddedValues, imageUrl, imagePath);
    }

    std::string Rest::GetImageDescription(std::string imageUrl, std::string imagePath)
    {
        //make JSON post field data (image url example: http://{ip_address}:{port}/data/image.png)
        std::string filename = fs::path(imagePath).filename().string(); //filename example: image.png
        std::string postField =
            "{\"data\":[{\"text\":\"describe the person with action, gender, clothing in one sentence\",\"files\":[{\"path\":\"" + imageUrl + "\",\"meta\":{\"_type\":\"gradio.FileData\"},\"orig_name\":\"" + filename + "\",\"url\":\"" + imageUrl + "\"}]},{\"history\":\"\"}]}";

        CCurlHelper::UrlInfo3 ReqInfo1{
           CCurlHelper::HttpRequestType::HTTP_POST, /*HTTP method*/
           m_hfHostname, /* Hugging Face Hostname (example: {..}.hf.space ) */ 
           0, /* Hugging Face port: not required */
           "", /* Hugging Face username: not required */
           m_hfApiKey, /* Hugging Face token key: (example: ab1cde12345f0ghkl61..*/
           m_hfResourcePath, /* Hugging Face resource path (example: /gradio_api/call/chat) */
           "", /* additional resource parameters: not required */
           postField, /*POST field*/
           10, /*http connection timeout in seconds*/
           10 /*http response timeout in seconds*/
        };

        std::string jsonHttpResponse = "";
        if (m_helper.SendHttpRequestToHuggingFace(ReqInfo1, jsonHttpResponse) != "") {
            WriteLog(logFilename, "Rest::GetImageDescription(): failed to receive response from Hugging Face Server. Please, check your settings in the INI file : " + m_iniFilename);
            return ""; 
        }

        std::string eventID = "";
        ParseHuggingFaceEventID(jsonHttpResponse, eventID);

        if (eventID.empty()) {
            WriteLog(logFilename, "Rest::GetImageDescription(): invalid/empty event ID retrieved from the Hugging Server. The Event ID is required in order to get the image description from the Hugging Server.");
            return ""; 
        }

        CCurlHelper::UrlInfo3 ReqInfo2{
            CCurlHelper::HttpRequestType::HTTP_GET, /*HTTP method*/
            m_hfHostname, /* Hugging Face Hostname (example: {..}.hf.space ) */ 
            0,  /* Hugging Face port: not required */
            "",  /* Hugging Face username: not required */
            m_hfApiKey, /* Hugging Face token key: (example: ab1cde12345f0ghkl61..*/
            m_hfResourcePath + "/" + eventID,  /* Hugging Face resource path (example: /gradio_api/call/chat/{eventID}) */
            "", /* additional resource parameters: not required */
            "", /*POST field: not required */
            10, /*http connection timeout in seconds*/
            10 /*http response timeout in seconds*/
        };

        std::string descr = "";
        m_helper.SendHttpRequestToHuggingFace(ReqInfo2, descr);      

        //parse event image description
        std::size_t found = descr.find("[");
        if (found != std::string::npos) {
            descr = descr.substr(found + 1);
            descr = descr.erase(descr.find(", null]"), 7);
            ReplaceAll(descr, "\"", "");
            ReplaceAll(descr, "\n", "");
        }
        else
            descr = ""; // ignore invalid descriptions received from Hugging Face Server

        return descr;
    }

    void Rest::GetEmbeddings(std::string description, std::string& embeddings)
    {
        if (description.empty()) 
            return;

        std::string jsonPostData = "{\"text\":\"" + description + "\"}";
        ReplaceAll(jsonPostData, "\n", "");

        //CLIP http server currently only accepts "localhost" as ip address
        m_clipHostname = "localhost";

        CCurlHelper::UrlInfo3 url{
            CCurlHelper::HttpRequestType::HTTP_POST, /*HTTP method*/
            m_clipHostname, /*CLIP SERVER IP address*/
            m_clipPort,/*CLIP SERVER port*/
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

        std::string parseError = ParseEmbeddedTextualData(jsonHttpResponse, description, embeddings);
        if (!parseError.empty())
            WriteLog(logFilename, "RestAPI::GetEmbeddings(): parsing error has occurred. \n>> INPUT JSON : \n" + jsonHttpResponse + "\n>> PARSING ERROR: \n" + parseError);
    }

    bool Rest::SaveEmbeddingsToDB(std::string description, std::string embeddings, std::string imageUrl, std::string imagePath)
    {
        if (description.empty() || embeddings.empty() || imageUrl.empty() || imagePath.empty()) {
            WriteLog(logFilename, "Rest::SaveEmbeddingsToDB(): invalid input parameters.");
            return false;
        }

        if (!IsConnected()) {
            WriteLog(logFilename, "Rest::SaveEmbeddingsToDB(): DB connection is not currently established.");
            return false;
        }

        // prepare the query
        std::ostringstream queryCmd;
        queryCmd << "INSERT INTO descriptions_embeddings VALUES (";
        queryCmd << "'" << description << "', ";
        queryCmd << "VEC_FromText('" << embeddings << "'), ";
        queryCmd << "'" << imageUrl << "', ";
        queryCmd << "'" << imagePath << "')";

#ifdef _WIN32
        //insert embeddings into the DB
        if (conn && !queryCmd.str().empty()) {
            if (mysql_query(conn, queryCmd.str().c_str())) {
                WriteLog(logFilename, "Rest::SaveEmbeddingsToDB(): failed to insert embeddings for the following image description into the DB: " + description + ". ERROR INFO => " + std::string(mysql_error(conn)));
                return false;
            }

            //check for errors
            if (mysql_affected_rows(conn) <= 0) {
                WriteLog(logFilename, "Rest::SaveEmbeddingsToDB(): possible embeddings duplicate for the following image description already exists in the DB: " + description + ". ERROR INFO => " + std::string(mysql_error(conn)));
                return false;
            }
        }
#endif	
        return true;
    }

} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx