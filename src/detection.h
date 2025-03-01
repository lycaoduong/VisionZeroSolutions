#pragma once
#include <nx/sdk/analytics/rect.h>
#include <nx/sdk/uuid.h>
#include <string>
#include <memory>
#include <vector>

namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            // Class labels for the Detection model (dataset).
            extern const std::vector<std::string> kClasses;
            extern const std::vector<std::string> kClassesToDetect;

            struct Detection
            {
                nx::sdk::analytics::Rect boundingBox;
                std::string classLabel;
                float confidence;
                nx::sdk::Uuid trackId;
                int IdxID;
                std::string clip_state;
            };

            using DetectionList = std::vector<std::shared_ptr<Detection>>;

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx