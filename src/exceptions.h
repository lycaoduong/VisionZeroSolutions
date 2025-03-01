#pragma once

// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <stdexcept>

#include <opencv2/core/core.hpp>

namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            class Error : public std::runtime_error { using std::runtime_error::runtime_error; };

            class ActionDetectorError : public Error { using Error::Error; };

            class ActionDetectorInitializationError : public ActionDetectorError
            {
                using ActionDetectorError::ActionDetectorError;
            };

            class ActionDetectorIsTerminatedError : public ActionDetectorError
            {
                using ActionDetectorError::ActionDetectorError;
            };

            class ObjectDetectionError : public ActionDetectorError
            {
                using ActionDetectorError::ActionDetectorError;
            };

            inline std::string cvExceptionToStdString(const cv::Exception& e)
            {
                return "OpenCV error: " + e.err + " (error code: " + std::to_string(e.code) + ")";
            }

            class ObjectTrackerError : public Error { using Error::Error; };
            class ObjectTrackingError : public ObjectTrackerError { using ObjectTrackerError::ObjectTrackerError; };

        } // namespace lcd_vision_solutions
    } // namespace vms_server_plugins
} // namespace nx_company