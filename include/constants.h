#ifndef SUBVISION_CORE_CONSTANTS_H
#define SUBVISION_CORE_CONSTANTS_H

#include <opencv2/opencv.hpp>

namespace subvision {
    const int SUBVISION_ZONE_TOP_LEFT = 0;
    const int SUBVISION_ZONE_TOP_RIGHT = 1;
    const int SUBVISION_ZONE_BOTTOM_LEFT = 2;
    const int SUBVISION_ZONE_BOTTOM_RIGHT = 3;
    const int SUBVISION_ZONE_CENTER = 4;
    const int SUBVISION_ZONE_UNDEFINED = -1;

    const int PICTURE_WIDTH_SHEET_DETECTION = 2000;
    const int PICTURE_HEIGHT_SHEET_DETECTION = 2000;
    const cv::Size KERNEL_SIZE(PICTURE_WIDTH_SHEET_DETECTION / 200, PICTURE_WIDTH_SHEET_DETECTION / 200);
    const cv::Mat ROUND_KERNEL = cv::getStructuringElement(cv::MORPH_ELLIPSE, KERNEL_SIZE);
}

#endif //SUBVISION_CORE_CONSTANTS_H
