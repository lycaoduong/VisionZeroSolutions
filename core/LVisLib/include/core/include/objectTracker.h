#ifndef OBJECT_TRACKER_H
#define OBJECT_TRACKER_H

#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <vector>

class ObjectTracker {
public:
    // Constructor
    ObjectTracker();

    // Create tracker by name
    cv::Ptr<cv::Tracker> createTrackerByName(std::string trackerType);

    // Initialize tracker with a frame and a bounding box
    bool initialize(const cv::Mat& frame, const cv::Rect2d& bbox);

    // Update tracker on a new frame
    bool update(const cv::Mat& frame);

    //Get tracking box
    cv::Rect getTrackBox();

    // Get missCount
    int getMissCount();

    // Draw the tracked points and bounding box on a frame
    void drawTracking(cv::Mat& frame) const;

    // // Check if the tracker is expired
    bool isExpired();

    // // Reset tracker age
    void resetAge();

    // Set max age
    void setMaxAge(int maxAge);

    // Get frame age

    int getAge();



private:
    cv::Ptr<cv::Tracker> tracker;       // OpenCV Tracker instance
    int frameAge;                       // Age of the tracker in frames
    int m_maxAge = 20;       // Maximum age before tracker expires
    cv::Rect trackedBox;              // Current tracked bounding box
    std::vector<cv::Point2d> trackPoints; // Stores center points of the bounding box
    // std::vector<cv::Rect> trackBoxes; // Stores center points of the bounding box
    int missTrackCount=0;

    std::vector<std::string> trackerTypes = { "BOOSTING", "MIL", "KCF", "TLD", "MEDIANFLOW", "GOTURN", "MOSSE", "CSRT" };
};

#endif // OBJECT_TRACKER_H
