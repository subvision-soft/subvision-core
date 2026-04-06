//
// Created by Paul on 15/06/2025.
//

#include "sheet_detection.h"
#include <opencv2/opencv.hpp>
#include <vector>

#include "constants.h"
#include "image_processing.h"
#include "utils.h"
#include "../include/logging.h"
#include "../include/logging.h"
using namespace cv;
namespace subvision {




    std::vector<Point2f> getSheetCoordinates(const Mat& sheet_mat) {
        log("getSheetCoordinates");
        const auto start = std::chrono::high_resolution_clock::now();
        Mat mat_resized;
        resize(sheet_mat, mat_resized, Size(PICTURE_WIDTH_SHEET_DETECTION, PICTURE_HEIGHT_SHEET_DETECTION));

        Mat hls;
        cvtColor(mat_resized, hls, COLOR_BGR2HLS);

        std::vector<Mat> channels(3);
        split(hls, channels);
        Mat &light = channels[1];

        double minVal, maxVal;
        minMaxLoc(light, &minVal, &maxVal);

        maxVal = std::max(maxVal, 120.0);
        minVal = (maxVal - minVal) * 0.5 + minVal;

        Mat mask;
        inRange(light, cv::Scalar(minVal), cv::Scalar(maxVal), mask);

        log("Start find contours");
        std::vector<std::vector<cv::Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        log("End find contours");

        const auto biggest = getBiggestValidContour(contours);
        log("Biggest contour size: " + std::to_string(biggest.size()));

        if (biggest.empty()) {
            log("No valid contour found");
            throw std::runtime_error("No valid contour found");
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        log("Temps écoulé pour getSheetCoordinates: " + std::to_string(elapsed.count()) + " secondes");

        return coordinatesToPercentage(biggest, PICTURE_WIDTH_SHEET_DETECTION, PICTURE_HEIGHT_SHEET_DETECTION);
    }

    // Recadrage du plastron à partir de l'image initiale
    Mat getSheetPicture(const Mat& image) {
        log("getSheetPicture");
        const auto coordinates = getSheetCoordinates(image);
        return getSheetPictureManually(image, coordinates);
    }

    // Recadrage manuel du plastron à partir de l'image initiale
    Mat getSheetPictureManually(const Mat& image,const std::vector<Point2f> coordinates){
        log("getSheetPictureManually");
        if (coordinates.empty()) {
            throw std::runtime_error("Sheet coordinates not found");
        }
        const int height = image.rows;
        const int width = image.cols;
        const auto real_coordinates = percentageToCoordinates(coordinates, width, height);
        const std::vector<cv::Point2f> target = {
            {0, 0},
            {PICTURE_WIDTH_SHEET_DETECTION, 0},
            {PICTURE_WIDTH_SHEET_DETECTION, PICTURE_HEIGHT_SHEET_DETECTION},
            {0, PICTURE_HEIGHT_SHEET_DETECTION}
        };
        if (real_coordinates.size() != 4 || target.size() != 4) {
            throw std::runtime_error("getPerspectiveTransform nécessite exactement 4 points source et 4 points cible.");
        }
        const Mat transform = getPerspectiveTransform(real_coordinates, target);
        Mat result;
        warpPerspective(image, result, transform, Size(PICTURE_WIDTH_SHEET_DETECTION, PICTURE_HEIGHT_SHEET_DETECTION));
        return result;
    }
}
