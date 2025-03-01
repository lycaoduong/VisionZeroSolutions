#pragma once

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <nx/sdk/analytics/i_uncompressed_video_frame.h>

namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {
            struct Frame
            {
                const int width;
                const int height;
                const int64_t timestampUs;
                const int64_t index;
                const cv::Mat cvMat;
            public:
                Frame(const nx::sdk::analytics::IUncompressedVideoFrame* frame, int64_t index) :
                    width(frame->width()),
                    height(frame->height()),
                    timestampUs(frame->timestampUs()),
                    index(index),
                    cvMat(
                    /*_rows*/ height,
                    /*_cols*/ width,
                    /*_type*/ CV_8UC3, //< BGR color space (default for OpenCV)
                    /*_data*/ (void*)frame->data(0),
                    /*_step*/ (size_t)frame->lineSize(0))
                {
                }
            };
                
        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx
