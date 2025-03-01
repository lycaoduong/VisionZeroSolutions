#include "CurlHelper.h"

#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include <cstring>

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

namespace nx {
namespace analytics {
namespace lcd_vision_solutions {

    CCurlHelper::CCurlHelper()
        : mHandles(nullptr)
        , mMultiHandle(nullptr)
    {

    }

    CCurlHelper::~CCurlHelper()
    {

    }

    std::pair<bool, std::string> CCurlHelper::PlaySpeakerSound(std::vector<UrlInfo> urls, int interval, IntervalUnit unit/* = IntervalUnit::MILLISECONDS*/)
    {
        std::lock_guard<std::mutex> lk(mMutex);

        uint64_t elaspedTime = 0;
        bool isTimerStart = mTimer.IsStart();
        if (!isTimerStart)
        {
            mTimer.Start();
        }
        else
        {
            switch (unit)
            {
            case CCurlHelper::IntervalUnit::MICROSECONDS:
                elaspedTime = mTimer.GetElapsedTime<CTimer::microseconds>().count();
                break;
            case CCurlHelper::IntervalUnit::MILLISECONDS:
                elaspedTime = mTimer.GetElapsedTime<CTimer::milliseconds>().count();
                break;
            case CCurlHelper::IntervalUnit::SECONDS:
                elaspedTime = mTimer.GetElapsedTime<CTimer::seconds>().count();
                break;
            case CCurlHelper::IntervalUnit::MINUTES:
                elaspedTime = mTimer.GetElapsedTime<CTimer::minutes>().count();
                break;
            case CCurlHelper::IntervalUnit::HOURS:
                elaspedTime = mTimer.GetElapsedTime<CTimer::hours>().count();
                break;
            default:
                break;
            }
        }

        if (isTimerStart && elaspedTime < (uint64_t)interval) // time elapsed
        {
            return std::pair<bool, std::string>();
        }
        else if (elaspedTime >= (uint64_t)interval)
        {
            mTimer.Stop();
            mTimer.Start();
        }

        auto size = static_cast<int>(urls.size());
        mHandles = new CURL * [size];
        std::stringstream ss;
        for (int i = 0; i < size; ++i)
        {
            auto& url = urls[i];

            // "http://127.0.0.1/cgi-bin/playclip.cgi?id=admin&pwd=aepel1234&location=7&repeat=1&volume=20"
            ss << url.addr;
            ss << "/cgi-bin/playclip.cgi?id=";
            ss << url.id;
            ss << "&pwd=";
            ss << url.password;
            ss << "&location=";
            ss << url.location;
            ss << "&repeat=";
            ss << url.repeat;
            ss << "&volume=";
            ss << url.volume;

            /* Allocate one CURL handle per transfer */
            mHandles[i] = curl_easy_init();
            if (!mHandles[i])           
                std::pair<bool, std::string>();

            /* set the options (I left out a few, you will get the point anyway) */
            curl_easy_setopt(mHandles[i], CURLOPT_URL, ss.str().c_str());
        }

        /* init a multi stack */
        mMultiHandle = curl_multi_init();

        if (!mMultiHandle)
            std::pair<bool, std::string>();

        /* add the individual transfers */
        for (int i = 0; i < size; i++)
        {
            curl_multi_add_handle(mMultiHandle, mHandles[i]);
        }

        int still_running = 1; /* keep number of running handles */
        while (still_running)
        {
            CURLMcode mc = curl_multi_perform(mMultiHandle, &still_running);
            if (still_running) {
                // wait for activity, timeout or "nothing"
                mc = curl_multi_poll(mMultiHandle, NULL, 0, 1000, NULL);
            }
            if (mc) {
                break;
            }
        }

        // remove the transfers and cleanup the mHandles
        for (int i = 0; i < size; i++) {
            curl_multi_remove_handle(mMultiHandle, mHandles[i]);
            curl_easy_cleanup(mHandles[i]);
            delete[] mHandles;
        }

        curl_multi_cleanup(mMultiHandle);

        return std::make_pair(true, ss.str());
    }

    std::pair<bool, std::string> CCurlHelper::ControlSignalTower(std::vector<UrlInfo2> urls, int interval, IntervalUnit unit/* = IntervalUnit::MILLISECONDS*/)
    {
        std::lock_guard<std::mutex> lk(mMutex);

        uint64_t elaspedTime = 0;
        bool isTimerStart = mTimer.IsStart();
        if (!isTimerStart)
        {
            mTimer.Start();
        }
        else
        {
            switch (unit)
            {
            case CCurlHelper::IntervalUnit::MICROSECONDS:
                elaspedTime = mTimer.GetElapsedTime<CTimer::microseconds>().count();
                break;
            case CCurlHelper::IntervalUnit::MILLISECONDS:
                elaspedTime = mTimer.GetElapsedTime<CTimer::milliseconds>().count();
                break;
            case CCurlHelper::IntervalUnit::SECONDS:
                elaspedTime = mTimer.GetElapsedTime<CTimer::seconds>().count();
                break;
            case CCurlHelper::IntervalUnit::MINUTES:
                elaspedTime = mTimer.GetElapsedTime<CTimer::minutes>().count();
                break;
            case CCurlHelper::IntervalUnit::HOURS:
                elaspedTime = mTimer.GetElapsedTime<CTimer::hours>().count();
                break;
            default:
                break;
            }
        }

        if (isTimerStart && elaspedTime < (uint64_t)interval) // time elapsed
        {
            return std::pair<bool, std::string>();
        }
        else if (elaspedTime >= (uint64_t)interval)
        {
            mTimer.Stop();
            mTimer.Start();
        }

        auto size = static_cast<int>(urls.size());
        mHandles = new CURL * [size];
        std::stringstream ss;
        for (int i = 0; i < size; ++i)
        {
            auto& url = urls[i];

            // http://127.0.0.1:80/api/control?alert=001110
            ss << "http://";
            ss << url.addr;
            ss << ":";
            ss << url.port;
            ss << url.resourcePath;
            ss << url.ledCode;

            /* Allocate one CURL handle per transfer */
            mHandles[i] = curl_easy_init();
            if (!mHandles[i])
                std::pair<bool, std::string>();

            /* set the options (I left out a few, you will get the point anyway) */
            curl_easy_setopt(mHandles[i], CURLOPT_URL, ss.str().c_str());
        }

        /* init a multi stack */
        mMultiHandle = curl_multi_init();

        if (!mMultiHandle)
            std::pair<bool, std::string>();

        /* add the individual transfers */
        for (int i = 0; i < size; i++)
        {
            curl_multi_add_handle(mMultiHandle, mHandles[i]);
        }

        int still_running = 1; /* keep number of running handles */
        while (still_running)
        {
            CURLMcode mc = curl_multi_perform(mMultiHandle, &still_running);
            if (still_running) {
                // wait for activity, timeout or "nothing"
                mc = curl_multi_poll(mMultiHandle, NULL, 0, 1000, NULL);
            }
            if (mc) {
                break;
            }
        }

        // remove the transfers and cleanup the mHandles
        for (int i = 0; i < size; i++) {
            curl_multi_remove_handle(mMultiHandle, mHandles[i]);
            curl_easy_cleanup(mHandles[i]);
            delete[] mHandles;
        }

        curl_multi_cleanup(mMultiHandle);

        return std::make_pair(true, ss.str());
    }

    //check http connection without performing any data transfer
    //use small timeout values such as 1 ~ 2 seconds because this will block the UI
    std::pair<bool, std::string> CCurlHelper::CheckConnection(std::string ipAddress, long timeout)
    {
        bool connectionStatus = false;

        //url format: http://192.168.0.200/
        std::stringstream url;
        url << "http://";
        url << ipAddress;
        url << "/";

        CURL* curl = curl_easy_init();
        if (curl) {
            CURLcode ret;
            curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
            curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout); // timeout in seconds
            ret = curl_easy_perform(curl);
            if (ret == CURLE_OK) {
                connectionStatus = true;
            }
        }

        return std::make_pair(connectionStatus, url.str());
    }

    static size_t WriteCallback(void* data, size_t size, size_t nmemb, void* clientp)
    {
        size_t realsize = size * nmemb;
        struct HttpMemory* mem = (struct HttpMemory*)clientp;

        char* ptr = (char*)realloc(mem->response, mem->size + realsize + 1);
        if (ptr == NULL)
            return 0;  /* out of memory! */

        mem->response = ptr;
        memcpy(&(mem->response[mem->size]), data, realsize);
        mem->size += realsize;
        mem->response[mem->size] = 0;

        return realsize;
    }
   
    // get embedding values from HTTP CLIP server
    std::string CCurlHelper::SendRequestToClipHttpServer(CCurlHelper::UrlInfo3 req, std::string& httpResponse)
    {
        std::string errorMsg = "";
        struct HttpMemory chunk = { 0 };

        std::string url = "http://" + req.ipAddress + ":" + std::to_string(req.port) + req.path;
        std::string postField = req.postfield; 
        int connTimeout = req.connectionTimeSec; 
        int resTimeout = req.responseTimeSec;

        CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (res != CURLE_OK) {
            errorMsg = "curl_global_init() error : " + std::string(curl_easy_strerror(res));
            return errorMsg;
        }

        CURL* curl = curl_easy_init();
        if (curl)
        {
            res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            struct curl_slist* hs = NULL;
            switch (req.type)
            {
            case HttpRequestType::HTTP_GET:
                res = curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));
                break;
            case HttpRequestType::HTTP_POST:
                res = curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

                res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)postField.length());
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

                res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postField.c_str());
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

                hs = curl_slist_append(hs, "Content-Type: application/json");
                res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));
                break;
            default:
                errorMsg = "unsupported <HttpRequestType> request type.";
                break;
            }

            res = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connTimeout);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, resTimeout);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            // maximum: CURL_MAX_READ_SIZE=524288, minimum size = 1024
            res = curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.83.1");
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_perform(curl);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));
            else {
                if (chunk.response) {
                    if (strlen(chunk.response) > 0) {
                        httpResponse = std::string(chunk.response);
                    }
                }
            }
            free(chunk.response);
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        return errorMsg;
    }   
   
    std::string CCurlHelper::SendHttpRequestToHuggingFace(CCurlHelper::UrlInfo3 req, std::string& httpResponse)
    {
        std::string errorMsg = "";
        struct HttpMemory chunk = { 0 };

        std::string url = "https://" + req.ipAddress + req.path; 
        std::string apiKey = req.password;  // Hugging Face Api Key
        std::string postField = req.postfield; 

        int connTimeout = req.connectionTimeSec; 
        int resTimeout = req.responseTimeSec; 

        CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (res != CURLE_OK) {
            errorMsg = "curl_global_init() error : " + std::string(curl_easy_strerror(res));
            return errorMsg;
        }

        CURL* curl = curl_easy_init();
        if (curl)
        {
            res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            struct curl_slist* hs = NULL;
            switch (req.type)
            {
            case CCurlHelper::HttpRequestType::HTTP_GET:
                res = curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

                break;
            case CCurlHelper::HttpRequestType::HTTP_POST:
                res = curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

                res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)postField.length());
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

                res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postField.c_str());
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

                hs = curl_slist_append(hs, std::string("Authorization: Bearer " + apiKey).c_str());
                hs = curl_slist_append(hs, "Content-Type: application/json");
                res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
                if (res != CURLE_OK)
                    errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

                break;
            default:
                errorMsg = "unsupported <HttpRequestType> request type.";
                break;
            }

            res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connTimeout);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, resTimeout);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            // maximum: CURL_MAX_READ_SIZE=524288, minimum size = 1024
            res = curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.83.1");
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
            if (res != CURLE_OK)
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                errorMsg = "curl_easy_setopt() error : " + std::string(curl_easy_strerror(res));
            }
            else {
                if (chunk.response) {
                    if (strlen(chunk.response) > 0) {
                        httpResponse = std::string(chunk.response);
                    }
                }
            }

            free(chunk.response);
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        return errorMsg;
    }

} // namespace lcd_vision_solutions
} // namespace analytics
} // namespace nx