#include <algorithm>
#include <map>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafeatures2d.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/tracking/tracking_by_matching.hpp>

#include "onnxruntime_cxx_api.h"
#include "opencv2/opencv.hpp"
#include <opencv2/tracking.hpp>


#define HAS_CPP17
#ifdef HAS_CPP17 // Use fs::
#include <filesystem>
//namespace fs = std::filesystem;
#else //HAS_CPP14
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif 
#define YoloV8
