#include "detection.h"

namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            // Class labels for the Model.
            // Pretrained YOLOv8 - 80 classes
            const std::vector<std::string> kClasses{
                "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light", 
                "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow", 
                "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", 
                "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", 
                "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", 
                "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", 
                "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", 
                "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", 
                "hair drier", "toothbrush"
                };
            // Filter object to detect
            const std::vector<std::string> kClassesToDetect{"person"};

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx