#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

#include "image_processing.h"
#include "sheet_detection.h"


int main() {
    std::cout << "Subvision CV - Development Play Ground" << std::endl;
    // CODE TO TEST

    cv::VideoCapture cap(0);
    while (true) {
        cv::Mat frame;
        cap >> frame;
        subvision::getSheetCoordinatesUsingAruCoMarkers(frame);
        cv::imshow("frame", frame);
        if (cv::waitKey(1) == 27)
            break;

    }

    // END OF CODE TO TEST
    return 0;
}

