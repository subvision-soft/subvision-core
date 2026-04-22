//
// Created by Paul on 15/06/2025.
//

#include "sheet_detection.h"
#include <opencv2/opencv.hpp>
#if __has_include(<opencv2/aruco.hpp>)
#include <opencv2/aruco.hpp>
#define SUBVISION_HAS_OPENCV_ARUCO 1
#elif __has_include(<opencv2/objdetect/aruco_detector.hpp>)
#include <opencv2/objdetect/aruco_detector.hpp>
#define SUBVISION_HAS_OPENCV_ARUCO 1
#else
#define SUBVISION_HAS_OPENCV_ARUCO 0
#endif
#include <vector>

#include "constants.h"
#include "image_processing.h"
#include "utils.h"
#include "../include/logging.h"
using namespace cv;

namespace subvision {
    std::vector<Point2f> getSheetCoordinatesUsingAruCoMarkers(const Mat &sheet_mat) {
        log("getSheetCoordinatesUsingAruCoMarkers");

#if SUBVISION_HAS_OPENCV_ARUCO

        cv::aruco::Dictionary dictionary =
                cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);

        Mat gray;
        cvtColor(sheet_mat, gray, COLOR_BGR2GRAY);

        std::vector<int> ids;
        std::vector<std::vector<Point2f> > corners;

        cv::aruco::DetectorParameters params;

        params.adaptiveThreshWinSizeMin = 5;
        params.adaptiveThreshWinSizeMax = 25;
        params.adaptiveThreshWinSizeStep = 10;

        params.adaptiveThreshConstant = 7;

        params.minMarkerPerimeterRate = 0.03;
        params.maxMarkerPerimeterRate = 4.0;

        params.polygonalApproxAccuracyRate = 0.03;

        params.cornerRefinementMethod =
                cv::aruco::CORNER_REFINE_SUBPIX;
        cv::aruco::ArucoDetector detector(dictionary, params);

        detector.detectMarkers(gray, corners, ids);
        Mat debug = sheet_mat.clone();

        if (!ids.empty()) {
            cv::aruco::drawDetectedMarkers(debug, corners, ids);
        }

        if (ids.empty()) {
            std::cout << "No markers detected\n";
            return {};
        }

        std::vector<Point2f> pts_f(4);
        std::vector found(4, false);

        for (size_t i = 0; i < ids.size(); i++) {
            int id = ids[i];
            if (id < 0 || id > 3) continue;

            const auto &c = corners[i];
            Point2f tl = c[0];
            Point2f tr = c[1];
            Point2f br = c[2];
            Point2f bl = c[3];

            Point2f xAxis = tr - tl;
            Point2f yAxis = bl - tl;

            float w = norm(xAxis);
            float h = norm(yAxis);

            if (w < 1e-6 || h < 1e-6) continue;

            xAxis /= w;
            yAxis /= h;

            Point2f p;

            switch (id) {
                case 0:
                    p = bl - yAxis * (-0.25f * h);
                    break;

                case 1:
                    p = br - yAxis * (-0.25f * h);
                    break;

                case 2:
                    p = tr + yAxis * (-0.25f * h);
                    break;

                case 3:
                    p = tl + yAxis * (-0.25f * h);
                    break;
            }

            pts_f[id] = p;
            found[id] = true;
        }

        for (int i = 0; i < 4; i++) {
            if (!found[i]) {
                std::cout << "Missing marker " << i << "\n";
                return {};
            }
        }

        std::vector<Point> pts;
        pts.reserve(4);

        for (int i = 0; i < 4; i++) {
            pts.emplace_back(cvRound(pts_f[i].x), cvRound(pts_f[i].y));
        }
        return coordinatesToPercentage(
            pts,
            PICTURE_WIDTH_SHEET_DETECTION,
            PICTURE_HEIGHT_SHEET_DETECTION
        );
#else
        // Keep behavior stable when OpenCV is built without ArUco (common in CI images).
        log("OpenCV ArUco module not available, fallback to contour-based sheet detection");
        return getSheetCoordinates(sheet_mat);
#endif
    }

    std::vector<Point2f> getSheetCoordinates(const Mat &sheet_mat) {
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
        inRange(light, Scalar(minVal), Scalar(maxVal), mask);

        log("Start find contours");
        std::vector<std::vector<Point> > contours;
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
    Mat getSheetPicture(const Mat &image) {
        log("getSheetPicture");
        const auto coordinates = getSheetCoordinates(image);
        return getSheetPictureManually(image, coordinates);
    }

    // Recadrage manuel du plastron à partir de l'image initiale
    Mat getSheetPictureManually(const Mat &image, const std::vector<Point2f> coordinates) {
        log("getSheetPictureManually");
        if (coordinates.empty()) {
            throw std::runtime_error("Sheet coordinates not found");
        }
        const int height = image.rows;
        const int width = image.cols;
        const auto real_coordinates = percentageToCoordinates(coordinates, width, height);
        const std::vector<Point2f> target = {
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
