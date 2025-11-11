#include "../include/target_detection.h"

#include <future>

#include "../include/constants.h"
#include "../include/utils.h"
#include "../include/image_processing.h"

namespace subvision {
    Ellipse getTargetEllipse(const cv::Mat &mat) {
        const auto start = std::chrono::high_resolution_clock::now();

        cv::Mat circle = cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);
        const cv::Point centerPoint(mat.cols / 2, mat.rows / 2);
        const int radius = static_cast<int>(mat.cols / 2.2);
        cv::circle(circle, centerPoint, radius, cv::Scalar(255), -1);

        cv::Mat xyz;
        cvtColor(mat, xyz, cv::COLOR_BGR2XYZ);
        std::vector<cv::Mat> xyzChannels(3);
        split(xyz, xyzChannels);
        cv::Mat &value = xyzChannels[2];

        bitwise_not(value, value);
        double minVal, maxVal;
        minMaxLoc(value, &minVal, &maxVal);
        minVal = maxVal - (maxVal - minVal) / 1.5;

        cv::Mat valueMask;
        inRange(value, cv::Scalar(minVal), cv::Scalar(maxVal), valueMask);

        const cv::Mat impacts = getImpactsMask(mat);
        cv::Mat notImpacts;
        bitwise_not(impacts, notImpacts);
        bitwise_and(valueMask, notImpacts, valueMask);

        cv::Mat close, element;
        cv::erode(valueMask, close, element, cv::Point(-1, -1), 10);
        cv::dilate(close, close, element, cv::Point(-1, -1), 20);
        cv::erode(close, close, element, cv::Point(-1, -1), 10);

        Ellipse ellipse = retrieveEllipse(close);

        auto ellipseIsValid = [](const Ellipse &e) {
            const float w = std::get<1>(e).width;
            const float h = std::get<1>(e).height;
            return w >= h * 0.7f && w <= h * 1.3f;
        };

        std::vector<cv::Point> ellipsePoints;
        ellipsePoints.reserve(360);

        try {
            cv::Mat empty = cv::Mat::zeros(mat.size(), mat.type());
            const cv::Point center = tupleIntCast(std::get<0>(ellipse));
            const cv::Size2f size = std::get<1>(ellipse);
            const float angle = std::get<2>(ellipse);

            ellipsePoints.clear();
            ellipse2Poly(center, cv::Size2f(size.width * 0.5f, size.height * 0.5f), static_cast<int>(angle), 0, 360, 1,
                         ellipsePoints);
            fillConvexPoly(empty, ellipsePoints, cv::Scalar(255, 255, 255));

            cv::Mat emptyGray;
            cvtColor(empty, emptyGray, cv::COLOR_BGR2GRAY);

            cv::Mat xor_;
            bitwise_xor(emptyGray, close, xor_);
            bitwise_or(close, xor_, close);

            ellipse = retrieveEllipse(close);

            if (!ellipseIsValid(ellipse)) {
                throw std::runtime_error("Problem during visual detection 1");
            }
        } catch (const std::exception &) {
            cv::Mat empty = cv::Mat::zeros(mat.size(), mat.type());
            const cv::Point center = tupleIntCast(std::get<0>(ellipse));
            const cv::Size2f size = std::get<1>(ellipse);
            const float angle = std::get<2>(ellipse);

            ellipsePoints.clear();
            ellipse2Poly(center, cv::Size2f(size.width * 0.5f, size.height * 0.5f), static_cast<int>(angle), 0, 360, 1,
                         ellipsePoints);
            fillConvexPoly(empty, ellipsePoints, cv::Scalar(255, 255, 255));

            cv::Mat emptyGray;
            cvtColor(empty, emptyGray, cv::COLOR_BGR2GRAY);

            bitwise_and(close, emptyGray, close);

            ellipse = retrieveEllipse(close);

            if (!ellipseIsValid(ellipse)) {
                throw std::runtime_error("Problem during visual detection");
            }
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour getTargetEllipse: " << elapsed.count() << " secondes" << std::endl;
        return ellipse;
    }

    Ellipse getTargetEllipseForZone(const cv::Mat &image, int zone) {
        return getTargetEllipse(getTargetPicture(image, zone));
    }

    std::map<int, Ellipse> getTargetsEllipse(const cv::Mat &image) {
        const std::vector<int> zones = {
            SUBVISION_ZONE_TOP_LEFT, SUBVISION_ZONE_TOP_RIGHT, SUBVISION_ZONE_CENTER,
            SUBVISION_ZONE_BOTTOM_LEFT, SUBVISION_ZONE_BOTTOM_RIGHT
        };

        std::map<int, Ellipse> ellipses;

        for (const auto &zone: zones) {
            ellipses[zone] = getTargetEllipseForZone(image, zone);
        }

        return ellipses;
    }

    std::map<int, Ellipse> targetCoordinatesToSheetCoordinates(const std::map<int, Ellipse> &ellipses) {
        std::map<int, Ellipse> newEllipses;

        for (const auto &[key, value]: ellipses) {
            cv::Point2f center = std::get<0>(value);
            cv::Size2f size = std::get<1>(value);
            float angle = std::get<2>(value);

            if (key == SUBVISION_ZONE_TOP_LEFT) {
                newEllipses[key] = std::make_tuple(center, size, angle);
            } else if (key == SUBVISION_ZONE_BOTTOM_LEFT) {
                newEllipses[key] = std::make_tuple(
                    cv::Point2f(center.x, center.y + PICTURE_HEIGHT_SHEET_DETECTION / 2),
                    size, angle
                );
            } else if (key == SUBVISION_ZONE_TOP_RIGHT) {
                newEllipses[key] = std::make_tuple(
                    cv::Point2f(center.x + PICTURE_WIDTH_SHEET_DETECTION / 2, center.y),
                    size, angle
                );
            } else if (key == SUBVISION_ZONE_BOTTOM_RIGHT) {
                newEllipses[key] = std::make_tuple(
                    cv::Point2f(center.x + PICTURE_WIDTH_SHEET_DETECTION / 2,
                                center.y + PICTURE_HEIGHT_SHEET_DETECTION / 2),
                    size, angle
                );
            } else if (key == SUBVISION_ZONE_CENTER) {
                newEllipses[key] = std::make_tuple(
                    cv::Point2f(center.x + PICTURE_WIDTH_SHEET_DETECTION / 4,
                                center.y + PICTURE_HEIGHT_SHEET_DETECTION / 4),
                    size, angle
                );
            }
        }

        return newEllipses;
    }

    void drawTargets(const std::map<int, Ellipse> &coordinates, cv::Mat &sheetMat) {
        constexpr int drawingWidth = 1;
        const cv::Scalar targetColor(0, 0, 255);
        constexpr float pi = 3.14159265f;
        constexpr float halfPi = pi * 0.5f;

        for (const auto &[_key, ellipseContrat]: coordinates) {
            const Ellipse ellipseCrossTip = growEllipse(ellipseContrat, 2.2f);
            const Ellipse ellipseMouche = growEllipse(ellipseContrat, 0.2f);
            const Ellipse ellipsePetitBlanc = growEllipse(ellipseContrat, 0.6f);
            const Ellipse ellipseMoyenBlanc = growEllipse(ellipseContrat, 1.4f);
            const Ellipse ellipseGrandBlanc = growEllipse(ellipseContrat, 1.8f);

            const cv::Point center = tupleIntCast(std::get<0>(ellipseContrat));
            const cv::Size2f size = std::get<1>(ellipseContrat);
            const float angle = std::get<2>(ellipseContrat);

            ellipse(sheetMat, center, cv::Size2f(size.width * 0.5f, size.height * 0.5f), angle, 0, 360, targetColor, drawingWidth);

            const cv::Point centerMouche = tupleIntCast(std::get<0>(ellipseMouche));
            const cv::Size2f sizeMouche = std::get<1>(ellipseMouche);
            const float angleMouche = std::get<2>(ellipseMouche);

            ellipse(sheetMat, centerMouche, cv::Size2f(sizeMouche.width * 0.5f, sizeMouche.height * 0.5f), angleMouche, 0, 360, targetColor, drawingWidth);

            const cv::Point centerPetitBlanc = tupleIntCast(std::get<0>(ellipsePetitBlanc));
            const cv::Size2f sizePetitBlanc = std::get<1>(ellipsePetitBlanc);
            const float anglePetitBlanc = std::get<2>(ellipsePetitBlanc);

            ellipse(sheetMat, centerPetitBlanc, cv::Size2f(sizePetitBlanc.width * 0.5f, sizePetitBlanc.height * 0.5f), anglePetitBlanc, 0, 360, targetColor, drawingWidth);

            const cv::Point centerMoyenBlanc = tupleIntCast(std::get<0>(ellipseMoyenBlanc));
            const cv::Size2f sizeMoyenBlanc = std::get<1>(ellipseMoyenBlanc);
            const float angleMoyenBlanc = std::get<2>(ellipseMoyenBlanc);

            ellipse(sheetMat, centerMoyenBlanc, cv::Size2f(sizeMoyenBlanc.width * 0.5f, sizeMoyenBlanc.height * 0.5f), angleMoyenBlanc, 0, 360, targetColor, drawingWidth);

            const cv::Point centerGrandBlanc = tupleIntCast(std::get<0>(ellipseGrandBlanc));
            const cv::Size2f sizeGrandBlanc = std::get<1>(ellipseGrandBlanc);
            const float angleGrandBlanc = std::get<2>(ellipseGrandBlanc);

            ellipse(sheetMat, centerGrandBlanc, cv::Size2f(sizeGrandBlanc.width * 0.5f, sizeGrandBlanc.height * 0.5f), angleGrandBlanc, 0, 360, targetColor, drawingWidth);

            const cv::Point2f topPoint = getPointOnEllipse(ellipseCrossTip, halfPi);
            const cv::Point2f bottomPoint = getPointOnEllipse(ellipseCrossTip, pi + halfPi);
            const cv::Point2f leftPoint = getPointOnEllipse(ellipseCrossTip, pi);
            const cv::Point2f rightPoint = getPointOnEllipse(ellipseCrossTip, 0.0f);

            line(sheetMat, tupleIntCast(topPoint), tupleIntCast(bottomPoint), targetColor, drawingWidth);
            line(sheetMat, tupleIntCast(leftPoint), tupleIntCast(rightPoint), targetColor, drawingWidth);
        }
    }

    void drawDetectedSheet(cv::Mat &sheetMat) {
    }
}
