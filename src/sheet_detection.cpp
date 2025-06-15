//
// Created by Paul on 15/06/2025.
//

#include "sheet_detection.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <optional>

#include "constants.h"
#include "image_processing.h"
#include "utils.h"
using namespace cv;
using namespace std;
namespace subvision {




    // Détection des coordonnées du plastron (par seuillage de la luminosité)
    std::vector<Point2f> getSheetCoordinates(const Mat& sheet_mat) {
        auto start = std::chrono::high_resolution_clock::now();
        Mat mat_resized;
        resize(sheet_mat, mat_resized, Size(PICTURE_WIDTH_SHEET_DETECTION, PICTURE_HEIGHT_SHEET_DETECTION));
        Mat hls;
        cvtColor(mat_resized, hls, COLOR_BGR2HLS);
        std::vector<Mat> channels;
        split(hls, channels);
        Mat light = channels[1];

        // Seuillage simple, à ajuster selon vos images
        Mat mask;
        double minVal, maxVal;
        minMaxLoc(light, &minVal, &maxVal);

        maxVal = std::max(maxVal, 120.0);
        minVal = (maxVal - minVal) / 2 + minVal;


        inRange(light, cv::Scalar(minVal), cv::Scalar(maxVal), mask);
        std::cout << "Start find contours" << std::endl;
        std::vector<std::vector<cv::Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        std::cout << "End find contours" << std::endl;

        const auto biggest = getBiggestValidContour(contours);
        std::cout << "Biggest contour size: " << biggest.size() << std::endl;
        if (biggest.empty()) {
            cout << "No valid contour found" << endl;
            throw std::runtime_error("No valid contour found");
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour getSheetCoordinates: " << elapsed.count() << " secondes" << std::endl;
        return coordinatesToPercentage(biggest, PICTURE_WIDTH_SHEET_DETECTION, PICTURE_HEIGHT_SHEET_DETECTION);
    }

    // Recadrage du plastron à partir de l'image initiale
    Mat getSheetPicture(const Mat& image) {
        const auto coordinates = getSheetCoordinates(image);
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
