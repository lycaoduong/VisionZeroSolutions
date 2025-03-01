#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <opencv2/core/core.hpp>
#include "VinEncrypt.h"
#include "coreInterface.h"


int sum( int a, int b );

std::vector<std::string> split(std::string inputStr, char delimiter);

std::vector<cv::Point> roi2contour(std::string roi);

std::vector<std::vector<cv::Point>> NormContour2contour(std::vector<std::vector<cv::Point>> cnt, cv::Mat frame);

void decryptModel(const char* fileIn, void const*& mBuffer, int& bufferSize);