#include "../include/image_processing.h"
#include "../include/constants.h"
#include "../include/utils.h"

namespace subvision {

    std::vector<cv::Point> getBiggestValidContour(const std::vector<std::vector<cv::Point>> &contours) {
        std::vector<cv::Point> biggestContour;
        double biggestArea = 0;

        for (const auto &contour: contours) {
            std::vector<cv::Point> approx;
            double epsilon = 0.01 * arcLength(contour, true);
            approxPolyDP(contour, approx, epsilon, true);

            if (approx.size() != 4 || contourArea(approx) < biggestArea) {
                continue;
            }

            // Check angles
            bool validAngles = true;
            for (int i = 0; i < 4; i++) {
                cv::Point p1 = approx[i];
                cv::Point p2 = approx[(i + 1) % 4];
                cv::Point p3 = approx[(i + 2) % 4];

                float dot = (p1.x - p2.x) * (p3.x - p2.x) + (p1.y - p2.y) * (p3.y - p2.y);
                float mag1 = sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
                float mag2 = sqrt(pow(p3.x - p2.x, 2) + pow(p3.y - p2.y, 2));
                float angle = acos(clamp(dot / (mag1 * mag2),-1.0f,1.0f)) * 180.0f / static_cast<float>(CV_PI);

                if (angle < 70.0f || angle > 110.0f) {
                    validAngles = false;
                    break;
                }
            }

            if (!validAngles) {
                continue;
            }

            double area = contourArea(approx);
            double totalArea = PICTURE_WIDTH_SHEET_DETECTION * PICTURE_HEIGHT_SHEET_DETECTION;

            if (area / totalArea < 0.1 || area / totalArea > 0.9) {
                continue;
            }

            biggestContour = approx;
            biggestArea = area;
        }

        return biggestContour;
    }

    cv::Mat getImpactsMask(const cv::Mat &image) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat hsv, mask;
        cvtColor(image, hsv, cv::COLOR_BGR2HSV);

        // Utiliser directement le canal de saturation sans split
        std::vector<cv::Mat> channels;
        split(hsv, channels);
        cv::Mat &saturation = channels[1];

        double minVal, maxVal;
        cv::minMaxLoc(saturation, &minVal, &maxVal);

        maxVal = std::max(maxVal, 120.0);
        minVal = (maxVal - minVal) / 2 + minVal;

        cv::inRange(saturation, cv::Scalar(minVal), cv::Scalar(maxVal), mask);

        // Utiliser un kernel plus petit pour accélérer l'opération morphologique
        static cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 1, cv::BORDER_REPLICATE);

        cv::threshold(mask, mask, 127, 255, cv::THRESH_BINARY);

        // Réduire le nombre de points pour fitEllipse
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Mat result = cv::Mat::zeros(mask.size(), mask.type());
        for (const auto &contour: contours) {
            if (contour.size() >= 5) {
                cv::RotatedRect ellipse = cv::fitEllipse(contour);
                std::vector<cv::Point> ellipsePoints;
                cv::ellipse2Poly(ellipse.center, cv::Size2f(ellipse.size.width / 2, ellipse.size.height / 2),
                                 static_cast<int>(ellipse.angle), 0, 360, 4, ellipsePoints);
                cv::fillConvexPoly(result, ellipsePoints, cv::Scalar(255));
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour getImpactsMask: " << elapsed.count() << " secondes" << std::endl;
        return result;
    }

    std::vector<cv::Point2f> getImpactsCoordinates(const cv::Mat &image) {
        cv::Mat mask = getImpactsMask(image);

        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        findContours(mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<cv::Point2f> centers;
        for (const auto &contour: contours) {
            if (contour.size() >= 5) {
                cv::RotatedRect ellipse = fitEllipse(contour);
                if (!isnan(ellipse.center.x) && !isnan(ellipse.center.y)) {
                    centers.push_back(ellipse.center);
                }
            }
        }

        return centers;
    }

    cv::Mat getColorMask(const cv::Mat &mat, const cv::Scalar &color) {
        cv::Mat colorMat(1, 1, CV_8UC3, color);
        cv::Mat hsv;
        cvtColor(colorMat, hsv, cv::COLOR_RGB2HSV);

        cv::Scalar minVal(hsv.at<cv::Vec3b>(0, 0)[0] - 10, 100, 50);
        cv::Scalar maxVal(hsv.at<cv::Vec3b>(0, 0)[0] + 10, 255, 255);

        cv::Mat hsvMat;
        cvtColor(mat, hsvMat, cv::COLOR_BGR2HSV);

        cv::Mat mask;
        inRange(hsvMat, minVal, maxVal, mask);

        cv::Mat kernel = getStructuringElement(
            cv::MORPH_RECT, cv::Size(PICTURE_WIDTH_SHEET_DETECTION / 200, PICTURE_HEIGHT_SHEET_DETECTION / 200));
        morphologyEx(mask, mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 1);
        threshold(mask, mask, 127, 255, cv::THRESH_BINARY);

        return mask;
    }

    Ellipse retrieveEllipse(const cv::Mat &image) {
        std::vector<std::vector<cv::Point>> contours;
        findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        if (contours.empty()) {
            return std::make_tuple(cv::Point2f(0, 0), cv::Size2f(0, 0), 0.0f);
        }

        // Trouver le plus grand contour
        auto biggestContour = *std::max_element(contours.begin(), contours.end(),
            [](const std::vector<cv::Point> &a, const std::vector<cv::Point> &b) {
                return contourArea(a) < contourArea(b);
            });

        // Utiliser directement le contour pour fitEllipse si possible
        if (biggestContour.size() >= 5) {
            cv::RotatedRect rotatedRect = fitEllipse(biggestContour);
            return std::make_tuple(rotatedRect.center, rotatedRect.size, rotatedRect.angle);
        }

        // Sinon, extraire les bords pour tenter un fitEllipse
        cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
        drawContours(mask, std::vector<std::vector<cv::Point>>{biggestContour}, -1, cv::Scalar(255), -1);

        cv::Mat grad;
        cv::Sobel(mask, grad, CV_8U, 1, 1);
        cv::threshold(grad, grad, 0, 255, cv::THRESH_BINARY);

        std::vector<cv::Point> edgePoints;
        cv::findNonZero(grad, edgePoints);

        if (edgePoints.size() >= 5) {
            cv::RotatedRect rotatedRect = fitEllipse(edgePoints);
            return std::make_tuple(rotatedRect.center, rotatedRect.size, rotatedRect.angle);
        }

        return std::make_tuple(cv::Point2f(0, 0), cv::Size2f(0, 0), 0.0f);
    }
}
