#include "../include/image_processing.h"
#include "../include/constants.h"
#include "../include/utils.h"

namespace subvision {
    std::vector<cv::Point> getBiggestValidContour(const std::vector<std::vector<cv::Point> > &contours) {
        std::cout << "Start processing getBiggestValidContour with " << contours.size() << " contours" << std::endl;
        std::vector<cv::Point> biggestContour;
        double biggestArea = 0;
        double totalArea = PICTURE_WIDTH_SHEET_DETECTION * PICTURE_HEIGHT_SHEET_DETECTION;

        for (const auto &contour: contours) {
            std::cout <<  "Processing contour with size: " << contour.size() << std::endl;
            // Éviter d'allouer approx si le contour est trop petit
            if (contour.size() < 4)
                continue;

            double epsilon = 0.01 * cv::arcLength(contour, true);
            std::vector<cv::Point> approx;
            cv::approxPolyDP(contour, approx, epsilon, true);

            if (approx.size() != 4)
                continue;

            double area = cv::contourArea(approx);
            if (area < biggestArea)
                continue;

            // Vérifier la proportion d'aire avant de calculer les angles
            double areaRatio = area / totalArea;
            if (areaRatio < 0.1 || areaRatio > 0.9)
                continue;

            // Vérifier les angles (éviter les allocations inutiles)
            bool validAngles = true;
            for (int i = 0; i < 4; ++i) {
                const cv::Point &p1 = approx[i];
                const cv::Point &p2 = approx[(i + 1) % 4];
                const cv::Point &p3 = approx[(i + 2) % 4];

                float dx1 = static_cast<float>(p1.x - p2.x);
                float dy1 = static_cast<float>(p1.y - p2.y);
                float dx2 = static_cast<float>(p3.x - p2.x);
                float dy2 = static_cast<float>(p3.y - p2.y);

                float dot = dx1 * dx2 + dy1 * dy2;
                float mag1 = std::hypot(dx1, dy1);
                float mag2 = std::hypot(dx2, dy2);

                // Éviter la division par zéro
                if (mag1 < 1e-6f || mag2 < 1e-6f) {
                    validAngles = false;
                    break;
                }

                float angle = std::acos(clamp(dot / (mag1 * mag2), -1.0f, 1.0f)) * 180.0f / static_cast<float>(
                                  CV_PI);
                if (angle < 70.0f || angle > 110.0f) {
                    validAngles = false;
                    break;
                }
            }

            if (!validAngles)
                continue;

            biggestContour = std::move(approx);
            biggestArea = area;
        }

        std::cout << "Biggest contour found with size: " << biggestContour.size() << std::endl;

        return biggestContour;
    }

    cv::Mat getImpactsMask(const cv::Mat &image) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat hsv, mask;
        cvtColor(image, hsv, cv::COLOR_BGR2HSV);

        std::vector<cv::Mat> channels;
        split(hsv, channels);
        cv::Mat &saturation = channels[1];

        double minVal, maxVal;
        cv::minMaxLoc(saturation, &minVal, &maxVal);

        maxVal = std::max(maxVal, 120.0);
        minVal = (maxVal - minVal) / 2 + minVal;

        cv::inRange(saturation, cv::Scalar(minVal), cv::Scalar(maxVal), mask);

        cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
        cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);


        threshold(mask, mask, 127, 255, cv::THRESH_BINARY);

        std::vector<std::vector<cv::Point> > contours;
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
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat mask = getImpactsMask(image);

        std::vector<std::vector<cv::Point> > contours;
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
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour getImpactsCoordinates: " << elapsed.count() << " secondes" << std::endl;
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
        cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
        cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
        threshold(mask, mask, 127, 255, cv::THRESH_BINARY);

        return mask;
    }

    Ellipse retrieveEllipse(const cv::Mat &image) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::vector<cv::Point> > contours;
        findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        if (contours.empty()) {
            return std::make_tuple(cv::Point2f(0, 0), cv::Size2f(0, 0), 0.0f);
        }

        // Trouver le plus grand contour
        size_t maxIdx = 0;
        double maxArea = 0.0;
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = contourArea(contours[i]);
            if (area > maxArea) {
                maxArea = area;
                maxIdx = i;
            }
        }
        const std::vector<cv::Point> &biggestContour = contours[maxIdx];

        // Utiliser directement le contour pour le fitEllipse si possible
        if (biggestContour.size() >= 5) {
            cv::RotatedRect rotatedRect = fitEllipse(biggestContour);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::cout << "Temps écoulé pour retrieveEllipse: " << elapsed.count() << " secondes" << std::endl;
            return std::make_tuple(rotatedRect.center, rotatedRect.size, rotatedRect.angle);
        }

        // Sinon, fallback sur l'ancienne méthode (rare)
        cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
        drawContours(mask, std::vector<std::vector<cv::Point> >{biggestContour}, -1, cv::Scalar(255), -1);

        std::vector<cv::Point> ptsEdges;
        findNonZero(mask, ptsEdges);

        if (ptsEdges.size() >= 5) {
            cv::RotatedRect rotatedRect = fitEllipse(ptsEdges);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::cout << "Temps écoulé pour retrieveEllipse: " << elapsed.count() << " secondes" << std::endl;
            return std::make_tuple(rotatedRect.center, rotatedRect.size, rotatedRect.angle);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour retrieveEllipse: " << elapsed.count() << " secondes" << std::endl;
        return std::make_tuple(cv::Point2f(0, 0), cv::Size2f(0, 0), 0.0f);
    }
}
