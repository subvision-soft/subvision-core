//
// Created by Paul on 15/06/2025.
//

#ifndef SHEET_DETECTION_H
#define SHEET_DETECTION_H
#include <opencv2/core/types.hpp>

namespace subvision {
    cv::Mat getSheetPicture(const cv::Mat& image) ;
    std::vector<cv::Point2f> getSheetCoordinates(const cv::Mat& sheet_mat) ;
}

#endif //SHEET_DETECTION_H
