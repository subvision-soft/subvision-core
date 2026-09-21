#include <cmath>
#include <vector>
#include <algorithm>

#include "utils.h"
#include "constants.h"
#include "logging.h"
namespace subvision {
    float toRadians(const float angle) {
        constexpr float degToRad = static_cast<float>(CV_PI) / 180.0f;
        return angle * degToRad;
    }

    float toDegrees(const float angle) {
        constexpr float radToDeg = 180.0f / static_cast<float>(CV_PI);
        return angle * radToDeg;
    }

    float clamp(const float value, const float min, const float max) {
        return std::max(min, std::min(max, value));
    }

    cv::Point tupleIntCast(const cv::Point2f &point) {
        return {static_cast<int>(point.x), static_cast<int>(point.y)};
    }

    cv::Point2f rotatePoint(const cv::Point2f &center, const cv::Point2f &point, const float angle) {
        const float s = sin(angle);
        const float c = cos(angle);

        const float translatedX = point.x - center.x;
        const float translatedY = point.y - center.y;

        const float rotatedX = translatedX * c - translatedY * s;
        const float rotatedY = translatedX * s + translatedY * c;

        return {rotatedX + center.x, rotatedY + center.y};
    }

    cv::Point2f getPointOnEllipse(const Ellipse &ellipse, const float angle) {
        const cv::Point2f &center = std::get<0>(ellipse);
        const cv::Size2f &radii = std::get<1>(ellipse);
        const float ellipseAngle = std::get<2>(ellipse);
        const float localAngle = toRadians(angle - ellipseAngle);
        const float x = center.x + cos(localAngle) * (radii.width / 2);
        const float y = center.y + sin(localAngle) * (radii.height / 2);
        return rotatePoint(center, {x, y}, toRadians(ellipseAngle));
    }

    constexpr float JUMP_THRESHOLD = 1.5;

    Ellipse growEllipse(const Ellipse &ellipse, const float factor) {
        const cv::Point2f &center = std::get<0>(ellipse);
        const cv::Size2f &radii = std::get<1>(ellipse);
        return std::make_tuple(center, cv::Size2f(radii.width * factor, radii.height * factor), std::get<2>(ellipse));
    }

    float getDistance(const cv::Point2f &point1, const cv::Point2f &point2) {
        const float dx = point1.x - point2.x;
        const float dy = point1.y - point2.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    float getAngle(const cv::Point2f &point1, const cv::Point2f &point2) {
        return atan2(point2.y - point1.y, point2.x - point1.x);
    }

    int getRealDistance(const cv::Point2f &center, const cv::Point2f &border, const cv::Point2f &impact) {
        const float length = getDistance(center, border);
        const float distance = getDistance(center, impact);
        const float percent = distance / length;
        constexpr float realLength = 45.0f;
        const float millimeterDistance = realLength * percent;
        return cvRound(millimeterDistance);
    }

     TargetSheetSpecs getTargetSheetSpecs(const Federation federation, const Event event) {
        for (const auto &specs: TARGET_SHEET_SPECS) {
            if (specs.federation == federation) {
                for (const auto &evt: specs.event) {
                    if (evt == event) {
                        return specs;
                    }
                }
            }
        }
        throw std::runtime_error("No matching target sheet specifications found.");
    }

    int getScore(const int distance, const Federation federation, const Event eventType) {

        const TargetSheetSpecs specs = getTargetSheetSpecs(federation, eventType);


        const std::list<AreaSpecs> areas = specs.targetSpecs.areas;

        if (const int maximumImpactDistance =  areas.back().radius; distance > maximumImpactDistance) {
            return 0;
        }
        const int max_score = specs.targetSpecs.maxScore;
        int score_to_subtract = 0;
        for (int i = 0; i < distance; ++i) {
            for (const auto &area: areas) {
                if (i <= area.radius) {
                    score_to_subtract += area.increment;
                }
            }
        }
        return max_score - score_to_subtract;
    }

    cv::Rect getCropCoordinates(const cv::Mat &image, int targetZone) {
        const int width = image.rows;
        const int height = image.cols;
        int x1 = 0, x2 = width, y1 = 0, y2 = height;

        if (targetZone == SUBVISION_ZONE_BOTTOM_LEFT || targetZone == SUBVISION_ZONE_BOTTOM_RIGHT) {
            x1 = width / 2;
        }
        if (targetZone == SUBVISION_ZONE_TOP_RIGHT || targetZone == SUBVISION_ZONE_BOTTOM_RIGHT) {
            y1 = height / 2;
        }
        if (targetZone == SUBVISION_ZONE_TOP_LEFT || targetZone == SUBVISION_ZONE_TOP_RIGHT) {
            x2 = width / 2;
        }
        if (targetZone == SUBVISION_ZONE_TOP_LEFT || targetZone == SUBVISION_ZONE_BOTTOM_LEFT) {
            y2 = height / 2;
        }
        if (targetZone == SUBVISION_ZONE_CENTER) {
            x1 = width / 4;
            y1 = height / 4;
            x2 = width - x1;
            y2 = height - y1;
        }

        return {y1, x1, y2 - y1, x2 - x1};
    }

    cv::Mat getTargetPicture(const cv::Mat &sheetMat, const int targetZone) {
        const cv::Rect coordinates = getCropCoordinates(sheetMat, targetZone);
        return sheetMat(coordinates).clone();
    }

    std::vector<cv::Point2f> coordinatesToPercentage(const std::vector<cv::Point> &coordinates, const int width,
                                                     const int height) {
        std::vector<cv::Point2f> percentageCoordinates;
        percentageCoordinates.reserve(coordinates.size());

        const float invWidth = 1.0f / static_cast<float>(width);
        const float invHeight = 1.0f / static_cast<float>(height);

        for (const auto &coordinate: coordinates) {
            percentageCoordinates.emplace_back(
                static_cast<float>(coordinate.x) * invWidth,
                static_cast<float>(coordinate.y) * invHeight
            );
        }
        subvision::log("Converted " + std::to_string(percentageCoordinates.size()) + " coordinates to percentage.");
        return percentageCoordinates;
    }

    std::vector<cv::Point2f> percentageToCoordinates(const std::vector<cv::Point2f> &percentageCoordinates,
                                                     const int width, const int height) {
        std::vector<cv::Point2f> coordinates;
        coordinates.reserve(percentageCoordinates.size());

        for (const auto &percentageCoordinate: percentageCoordinates) {
            coordinates.emplace_back(
                percentageCoordinate.x * static_cast<float>(width),
                percentageCoordinate.y * static_cast<float>(height)
            );
        }
        return coordinates;
    }

    std::vector<Impact> createImpactVector() {
        return {};
    }

    std::vector<cv::Point> cleanupEllipticalContour(const std::vector<cv::Point> &contour) {
        if (contour.size() < 5) {
            return contour;
        }

        const cv::RotatedRect ellipse = cv::fitEllipse(contour);

        const double ecx = ellipse.center.x;
        const double ecy = ellipse.center.y;
        const double a = ellipse.size.width / 2.0;
        const double b = ellipse.size.height / 2.0;
        const double angle_rad = ellipse.angle * CV_PI / 180.0;

        constexpr double THRESHOLD_PIXELS = PICTURE_HEIGHT_SHEET_DETECTION + PICTURE_WIDTH_SHEET_DETECTION / 500.0;

        std::vector<cv::Point> cleaned_contour;
        cleaned_contour.reserve(contour.size());

        for (const auto &pt : contour) {
            const double dx = pt.x - ecx;
            const double dy = pt.y - ecy;
            const double distance = std::sqrt(dx * dx + dy * dy);

            const double angle_pt = std::atan2(dy, dx);
            const double theta_local = angle_pt - angle_rad;

            const double cos_t = std::cos(theta_local);
            const double sin_t = std::sin(theta_local);
            const double expected_distance = (a * b) / std::sqrt((b * cos_t) * (b * cos_t) + (a * sin_t) * (a * sin_t));

            const double deviation = expected_distance - distance;

            if (deviation <= THRESHOLD_PIXELS) {
                cleaned_contour.push_back(pt);
            }
        }

        return cleaned_contour;
    }

}
