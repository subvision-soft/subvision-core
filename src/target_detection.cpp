#include "../include/target_detection.h"
#include "../include/constants.h"
#include "../include/utils.h"
#include "../include/image_processing.h"

namespace subvision {

    Ellipse getTargetEllipse(const cv::Mat &mat) {
        cv::Mat circle = cv::Mat::zeros(mat.cols, mat.rows, CV_8UC1);
        circle(cv::Rect(0, 0, mat.cols, mat.rows)) = 0;
        cv::circle(circle, cv::Point(mat.cols / 2, mat.rows / 2), static_cast<int>(mat.cols / 2.2), cv::Scalar(255), -1);

        cv::Mat xyz;
        cvtColor(mat.clone(), xyz, cv::COLOR_BGR2XYZ);

        std::vector<cv::Mat> xyzChannels;
        split(xyz, xyzChannels);
        cv::Mat value = xyzChannels[2];

        bitwise_not(value, value);

        double minVal, maxVal;
        minMaxLoc(value, &minVal, &maxVal);
        minVal = maxVal - (maxVal - minVal) / 1.5;

        cv::Mat valueMask;
        inRange(value, cv::Scalar(minVal), cv::Scalar(maxVal), valueMask);

        // bitwise_and(valueMask, circle, valueMask);

        cv::Mat impacts = getImpactsMask(mat);
        cv::Mat notImpacts;
        bitwise_not(impacts, notImpacts);
        bitwise_and(valueMask, notImpacts, valueMask);

        cv::Mat close;
        cv::erode(valueMask, close, cv::Mat(), cv::Point(-1, -1), 10);
        cv::dilate(close, close, cv::Mat(), cv::Point(-1, -1), 20);
        cv::erode(close, close, cv::Mat(), cv::Point(-1, -1), 10);

        Ellipse ellipse = retrieveEllipse(close);

        try {
            cv::Mat empty = cv::Mat::zeros(mat.size(), mat.type());
            cv::Point center = tupleIntCast(std::get<0>(ellipse));
            cv::Size size = cv::Size(static_cast<int>(std::get<1>(ellipse).width), static_cast<int>(std::get<1>(ellipse).height));
            float angle = std::get<2>(ellipse);

            std::vector<cv::Point> ellipsePoints;
            ellipse2Poly(center, cv::Size2f(size.width / 2, size.height / 2), static_cast<int>(angle), 0, 360, 1,
                        ellipsePoints);
            fillConvexPoly(empty, ellipsePoints, cv::Scalar(255, 255, 255));

            cv::Mat emptyGray;
            cvtColor(empty, emptyGray, cv::COLOR_BGR2GRAY);

            cv::Mat xor_;
            bitwise_xor(emptyGray, close, xor_);
            bitwise_or(close, xor_, close);

            ellipse = retrieveEllipse(close);

            if (std::get<1>(ellipse).width < std::get<1>(ellipse).height * 0.7 ||
                std::get<1>(ellipse).width > std::get<1>(ellipse).height * 1.3) {
                throw std::runtime_error("Problem during visual detection 1");
            }
        } catch (const std::exception &) {
            cv::Mat empty = cv::Mat::zeros(mat.size(), mat.type());
            cv::Point center = tupleIntCast(std::get<0>(ellipse));
            cv::Size size = cv::Size(static_cast<int>(std::get<1>(ellipse).width), static_cast<int>(std::get<1>(ellipse).height));
            float angle = std::get<2>(ellipse);

            std::vector<cv::Point> ellipsePoints;
            ellipse2Poly(center, cv::Size2f(size.width / 2, size.height / 2), static_cast<int>(angle), 0, 360, 1,
                        ellipsePoints);
            fillConvexPoly(empty, ellipsePoints, cv::Scalar(255, 255, 255));

            cv::Mat emptyGray;
            cvtColor(empty, emptyGray, cv::COLOR_BGR2GRAY);

            bitwise_and(close, emptyGray, close);

            cv::Mat matCopy = mat.clone();
            ellipse = retrieveEllipse(close);

            ellipse2Poly(tupleIntCast(std::get<0>(ellipse)),
                        cv::Size2f(std::get<1>(ellipse).width / 2, std::get<1>(ellipse).height / 2),
                        static_cast<int>(std::get<2>(ellipse)), 0, 360, 1, ellipsePoints);
            polylines(matCopy, ellipsePoints, true, cv::Scalar(0, 255, 0), 1);

            if (std::get<1>(ellipse).width < std::get<1>(ellipse).height * 0.7 ||
                std::get<1>(ellipse).width > std::get<1>(ellipse).height * 1.3) {
                throw std::runtime_error("Problem during visual detection");
            }
        }

        return ellipse;
    }

    Ellipse getTargetEllipseForZone(const cv::Mat &image, int zone) {
        return getTargetEllipse(getTargetPicture(image, zone));
    }

    std::map<int, Ellipse> getTargetsEllipse(const cv::Mat &image) {
        std::vector<int> zones = {
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

        for (const auto &pair: ellipses) {
            const auto &key = pair.first;
            const auto &value = pair.second;
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
        for (const auto &pair: coordinates) {
            const auto &key = pair.first;
            const auto &ellipseContrat = pair.second;
            int drawingWidth = 1;
            cv::Scalar targetColor(0, 0, 255);

            Ellipse ellipseCrossTip = growEllipse(ellipseContrat, 2.2f);
            Ellipse ellipseMouche = growEllipse(ellipseContrat, 0.2f);
            Ellipse ellipsePetitBlanc = growEllipse(ellipseContrat, 0.6f);
            Ellipse ellipseMoyenBlanc = growEllipse(ellipseContrat, 1.4f);
            Ellipse ellipseGrandBlanc = growEllipse(ellipseContrat, 1.8f);

            cv::Point center = tupleIntCast(std::get<0>(ellipseContrat));
            cv::Size size = cv::Size(static_cast<int>(std::get<1>(ellipseContrat).width),
                                static_cast<int>(std::get<1>(ellipseContrat).height));
            float angle = std::get<2>(ellipseContrat);

            ellipse(sheetMat, center, cv::Size2f(size.width / 2, size.height / 2), angle, 0, 360, targetColor,
                    drawingWidth);

            cv::Point centerMouche = tupleIntCast(std::get<0>(ellipseMouche));
            cv::Size sizeMouche = cv::Size(static_cast<int>(std::get<1>(ellipseMouche).width),
                                    static_cast<int>(std::get<1>(ellipseMouche).height));
            float angleMouche = std::get<2>(ellipseMouche);

            ellipse(sheetMat, centerMouche, cv::Size2f(sizeMouche.width / 2, sizeMouche.height / 2),
                    angleMouche, 0, 360, targetColor, drawingWidth);

            cv::Point centerPetitBlanc = tupleIntCast(std::get<0>(ellipsePetitBlanc));
            cv::Size sizePetitBlanc = cv::Size(static_cast<int>(std::get<1>(ellipsePetitBlanc).width),
                                        static_cast<int>(std::get<1>(ellipsePetitBlanc).height));
            float anglePetitBlanc = std::get<2>(ellipsePetitBlanc);

            ellipse(sheetMat, centerPetitBlanc, cv::Size2f(sizePetitBlanc.width / 2, sizePetitBlanc.height / 2),
                    anglePetitBlanc, 0, 360, targetColor, drawingWidth);

            cv::Point centerMoyenBlanc = tupleIntCast(std::get<0>(ellipseMoyenBlanc));
            cv::Size sizeMoyenBlanc = cv::Size(static_cast<int>(std::get<1>(ellipseMoyenBlanc).width),
                                        static_cast<int>(std::get<1>(ellipseMoyenBlanc).height));
            float angleMoyenBlanc = std::get<2>(ellipseMoyenBlanc);

            ellipse(sheetMat, centerMoyenBlanc, cv::Size2f(sizeMoyenBlanc.width / 2, sizeMoyenBlanc.height / 2),
                    angleMoyenBlanc, 0, 360, targetColor, drawingWidth);

            cv::Point centerGrandBlanc = tupleIntCast(std::get<0>(ellipseGrandBlanc));
            cv::Size sizeGrandBlanc = cv::Size(static_cast<int>(std::get<1>(ellipseGrandBlanc).width),
                                        static_cast<int>(std::get<1>(ellipseGrandBlanc).height));
            float angleGrandBlanc = std::get<2>(ellipseGrandBlanc);

            ellipse(sheetMat, centerGrandBlanc, cv::Size2f(sizeGrandBlanc.width / 2, sizeGrandBlanc.height / 2),
                    angleGrandBlanc, 0, 360, targetColor, drawingWidth);

            cv::Point2f topPoint = getPointOnEllipse(ellipseCrossTip, toRadians(90.0f));
            cv::Point2f bottomPoint = getPointOnEllipse(ellipseCrossTip, toRadians(270.0f));
            cv::Point2f leftPoint = getPointOnEllipse(ellipseCrossTip, toRadians(180.0f));
            cv::Point2f rightPoint = getPointOnEllipse(ellipseCrossTip, toRadians(0.0f));

            line(sheetMat, tupleIntCast(topPoint), tupleIntCast(bottomPoint), targetColor, drawingWidth);
            line(sheetMat, tupleIntCast(leftPoint), tupleIntCast(rightPoint), targetColor, drawingWidth);
        }
    }
}
