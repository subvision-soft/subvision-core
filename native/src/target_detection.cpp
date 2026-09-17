#include "target_detection.h"

#include <future>
#include <vector>
#include <ranges>

#include "constants.h"
#include "logging.h"
#include "utils.h"
#include "image_processing.h"

namespace subvision {
    Ellipse getTargetEllipse(const cv::Mat &mat) {
        log("getTargetEllipse");
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
        minVal = maxVal - (maxVal - minVal) / 1.6;

        cv::Mat valueMask;
        inRange(value, cv::Scalar(minVal), cv::Scalar(maxVal), valueMask);
        // cv::imshow("valueMask", valueMask);
        const cv::Mat impacts = getImpactsMask(mat);
        // cv::imshow("impacts", impacts);
        cv::Mat notImpacts;
        bitwise_not(impacts, notImpacts);
        bitwise_and(valueMask, notImpacts, valueMask);

        cv::Mat close, element;
        cv::erode(valueMask, close, element, cv::Point(-1, -1), 10);
        cv::dilate(close, close, element, cv::Point(-1, -1), 20);
        cv::erode(close, close, element, cv::Point(-1, -1), 10);
        // cv::imshow("close",close);
        Ellipse ellipse = retrieveEllipse(close);

        auto ellipseIsValid = [](const Ellipse &e) {
            const float w = std::get<1>(e).width;
            const float h = std::get<1>(e).height;
            return w >= h * 0.7f && w <= h * 1.3f;
        };

        std::vector<cv::Point> ellipsePoints;
        ellipsePoints.reserve(360);
        // cv::waitKey(0);
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
        subvision::log("Temps écoulé pour getTargetEllipse: " + std::to_string(elapsed.count()) + " secondes");
        return ellipse;
    }

    Ellipse getTargetEllipseForZone(const cv::Mat &image, int zone) {
        log("getTargetEllipseForZone");
        return getTargetEllipse(getTargetPicture(image, zone));
    }

    std::map<int, Ellipse> getTargetsEllipse(const cv::Mat &image) {
        log("getTargetsEllipse");
        const std::vector zones = {
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
        subvision::log("targetCoordinatesToSheetCoordinates");
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

    void drawTargets(const std::map<int, Ellipse> &coordinates, cv::Mat &sheetMat, const Federation federation,
                     const Event eventType) {
        log("drawTargets");
        constexpr int drawingWidth = 1;
        const cv::Scalar targetColor(0, 0, 255);
        const TargetSheetSpecs specs = getTargetSheetSpecs(federation, eventType);
        int main_area_radius;
        for (const auto &area: specs.targetSpecs.areas) {
            if (area.main) {
                main_area_radius = area.radius;
                break;
            }
        }
        for (const auto &ellipseContrat: coordinates | std::views::values) {
            const cv::Point center = tupleIntCast(std::get<0>(ellipseContrat));
            const cv::Size2f size = std::get<1>(ellipseContrat);
            const float angle = std::get<2>(ellipseContrat);

            ellipse(sheetMat, center, cv::Size2f(size.width * 0.5f, size.height * 0.5f), angle, 0, 360, targetColor,
                    drawingWidth);
            std::vector<float> ratios;
            for (const auto &area: specs.targetSpecs.areas) {
                float ratio = static_cast<float>(area.radius) / static_cast<float>(main_area_radius);
                const Ellipse area_ellipse = growEllipse(ellipseContrat, ratio);
                const cv::Point centerEllipse = tupleIntCast(std::get<0>(area_ellipse));
                const cv::Size2f sizeEllipse = std::get<1>(area_ellipse);
                const float angleEllipse = std::get<2>(area_ellipse);
                ellipse(sheetMat, centerEllipse, cv::Size2f(sizeEllipse.width * 0.5f, sizeEllipse.height * 0.5f),
                        angleEllipse,
                        0, 360, targetColor, drawingWidth);
            }
        }
    }
}
