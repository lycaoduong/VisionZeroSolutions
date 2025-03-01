#pragma once
#include "onnxruntime_cxx_api.h"

#define HAS_CPP17
#ifdef HAS_CPP17 // Use fs::
#include <filesystem>
//namespace fs = std::filesystem;
#else //HAS_CPP14
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif 


class OnnxLoader
{
public:
    OnnxLoader();
    struct Model
    {
        const OrtApi* m_ort;
        OrtSession* m_session;
    };
public:
    //bool loadModel(const fs::path fsmodelPath, std::string env_runtime, const char* gpu_index, Model& model);
    bool loadModel(const std::filesystem::path fsmodelPath, std::string env_runtime, const char* gpu_index, Model& model);
    bool loadModel(const void* model_data, size_t model_data_length, std::string env_runtime, const char* gpu_index, Model& model);
    void get_model_infor();
private:
    //std::pair<const OrtApi*, OrtSession*> core_ort;
    const OrtApi* ort_core;
    //OrtApi ort_core;
    OrtEnv* env;
    OrtThreadingOptions* envOpts = nullptr;
    OrtSession* session;
    OrtSessionOptions* session_options;
    OrtCUDAProviderOptionsV2* cuda_options = nullptr;
    OrtTensorRTProviderOptionsV2* trt_options = nullptr;
    std::string device = "cuda";
};

