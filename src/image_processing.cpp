#include "../include/image_processing.h"
#include "../include/constants.h"
#include "../include/utils.h"

namespace subvision {
    std::vector<cv::Point> getBiggestValidContour(const std::vector<std::vector<cv::Point> > &contours) {
        std::cout << "Start processing getBiggestValidContour with " << contours.size() << " contours" << std::endl;
        std::vector<cv::Point> biggestContour;
        double biggestArea = 0;
        constexpr double totalArea = PICTURE_WIDTH_SHEET_DETECTION * PICTURE_HEIGHT_SHEET_DETECTION;
        constexpr double minAreaRatio = 0.1;
        constexpr double maxAreaRatio = 0.9;
        constexpr float minAngle = 70.0f;
        constexpr float maxAngle = 110.0f;
        constexpr float invPI180 = 180.0f / static_cast<float>(CV_PI);

        std::vector<cv::Point> approx;
        approx.reserve(4);

        for (const auto &contour: contours) {
            std::cout <<  "Processing contour with size: " << contour.size() << std::endl;
            if (contour.size() < 4)
                continue;

            const double epsilon = 0.01 * cv::arcLength(contour, true);
            approx.clear();
            cv::approxPolyDP(contour, approx, epsilon, true);

            if (approx.size() != 4)
                continue;

            const double area = cv::contourArea(approx);
            if (area <= biggestArea)
                continue;

            const double areaRatio = area / totalArea;
            if (areaRatio < minAreaRatio || areaRatio > maxAreaRatio)
                continue;

            bool validAngles = true;
            for (int i = 0; i < 4; ++i) {
                const cv::Point &p1 = approx[i];
                const cv::Point &p2 = approx[(i + 1) % 4];
                const cv::Point &p3 = approx[(i + 2) % 4];

                const float dx1 = static_cast<float>(p1.x - p2.x);
                const float dy1 = static_cast<float>(p1.y - p2.y);
                const float dx2 = static_cast<float>(p3.x - p2.x);
                const float dy2 = static_cast<float>(p3.y - p2.y);

                const float dot = dx1 * dx2 + dy1 * dy2;
                const float mag1Sq = dx1 * dx1 + dy1 * dy1;
                const float mag2Sq = dx2 * dx2 + dy2 * dy2;

                if (mag1Sq < 1e-12f || mag2Sq < 1e-12f) {
                    validAngles = false;
                    break;
                }

                const float invMag = 1.0f / std::sqrt(mag1Sq * mag2Sq);
                const float cosAngle = clamp(dot * invMag, -1.0f, 1.0f);
                const float angle = std::acos(cosAngle) * invPI180;

                if (angle < minAngle || angle > maxAngle) {
                    validAngles = false;
                    break;
                }
            }

            if (!validAngles)
                continue;

            biggestContour = approx;
            biggestArea = area;
        }

        std::cout << "Biggest contour found with size: " << biggestContour.size() << std::endl;

        return biggestContour;
    }

    cv::Mat getImpactsMask(const cv::Mat &image) {
        const auto start = std::chrono::high_resolution_clock::now();
        cv::Mat hsv, mask;
        cvtColor(image, hsv, cv::COLOR_BGR2HSV);

        std::vector<cv::Mat> channels(3);
        split(hsv, channels);
        const cv::Mat &saturation = channels[1];

        double minVal, maxVal;
        cv::minMaxLoc(saturation, &minVal, &maxVal);

        maxVal = std::max(maxVal, 120.0);
        minVal = (maxVal - minVal) * 0.5 + minVal;

        cv::inRange(saturation, cv::Scalar(minVal), cv::Scalar(maxVal), mask);

        cv::Mat element;
        cv::erode(mask, mask, element, cv::Point(-1, -1), 2);
        cv::dilate(mask, mask, element, cv::Point(-1, -1), 2);

        threshold(mask, mask, 127, 255, cv::THRESH_BINARY);

        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Mat result = cv::Mat::zeros(mask.size(), mask.type());
        std::vector<cv::Point> ellipsePoints;
        ellipsePoints.reserve(90);

        for (const auto &contour: contours) {
            if (contour.size() >= 5) {
                const cv::RotatedRect ellipse = cv::fitEllipse(contour);
                ellipsePoints.clear();
                cv::ellipse2Poly(ellipse.center, cv::Size2f(ellipse.size.width * 0.5f, ellipse.size.height * 0.5f),
                                 static_cast<int>(ellipse.angle), 0, 360, 4, ellipsePoints);
                cv::fillConvexPoly(result, ellipsePoints, cv::Scalar(255));
            }
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour getImpactsMask: " << elapsed.count() << " secondes" << std::endl;
        return result;
    }

    std::vector<cv::Point2f> getImpactsCoordinates(const cv::Mat &image) {
        const auto start = std::chrono::high_resolution_clock::now();
        const cv::Mat mask = getImpactsMask(image);

        std::vector<std::vector<cv::Point> > contours;
        findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<cv::Point2f> centers;
        centers.reserve(contours.size());

        for (const auto &contour: contours) {
            if (contour.size() >= 5) {
                const cv::RotatedRect ellipse = fitEllipse(contour);
                if (!isnan(ellipse.center.x) && !isnan(ellipse.center.y)) {
                    centers.push_back(ellipse.center);
                }
            }
        }
        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour getImpactsCoordinates: " << elapsed.count() << " secondes" << std::endl;
        return centers;
    }

    cv::Mat getColorMask(const cv::Mat &mat, const cv::Scalar &color) {
        const cv::Mat colorMat(1, 1, CV_8UC3, color);
        cv::Mat hsv;
        cvtColor(colorMat, hsv, cv::COLOR_RGB2HSV);

        const cv::Scalar minVal(hsv.at<cv::Vec3b>(0, 0)[0] - 10, 100, 50);
        const cv::Scalar maxVal(hsv.at<cv::Vec3b>(0, 0)[0] + 10, 255, 255);

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
        const auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::vector<cv::Point> > contours;
        findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        Ellipse emptyEllipse = std::make_tuple(cv::Point2f(0, 0), cv::Size2f(0, 0), 0.0f);

        if (contours.empty()) {
            return emptyEllipse;
        }

        const auto maxIt = std::ranges::max_element(contours,
                                                    [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                                                        return contourArea(a) < contourArea(b);
                                                    });

        const std::vector<cv::Point> &biggestContour = *maxIt;

        if (biggestContour.size() >= 5) {
            const cv::RotatedRect rotatedRect = fitEllipse(biggestContour);
            const auto end = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double> elapsed = end - start;
            std::cout << "Temps écoulé pour retrieveEllipse: " << elapsed.count() << " secondes" << std::endl;
            return std::make_tuple(rotatedRect.center, rotatedRect.size, rotatedRect.angle);
        }

        cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
        drawContours(mask, std::vector<std::vector<cv::Point> >{biggestContour}, -1, cv::Scalar(255), -1);

        std::vector<cv::Point> ptsEdges;
        findNonZero(mask, ptsEdges);

        if (ptsEdges.size() >= 5) {
            const cv::RotatedRect rotatedRect = fitEllipse(ptsEdges);
            const auto end = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double> elapsed = end - start;
            std::cout << "Temps écoulé pour retrieveEllipse: " << elapsed.count() << " secondes" << std::endl;
            return std::make_tuple(rotatedRect.center, rotatedRect.size, rotatedRect.angle);
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour retrieveEllipse: " << elapsed.count() << " secondes" << std::endl;
        return emptyEllipse;
    }
}
