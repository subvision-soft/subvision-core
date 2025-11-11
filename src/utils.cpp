#include "../include/utils.h"
#include "../include/constants.h"

namespace subvision {

    inline float toRadians(const float angle) {
        constexpr float degToRad = static_cast<float>(CV_PI) / 180.0f;
        return angle * degToRad;
    }

    inline float toDegrees(const float angle) {
        constexpr float radToDeg = 180.0f / static_cast<float>(CV_PI);
        return angle * radToDeg;
    }

    inline float clamp(const float value, const float min, const float max) {
        return std::max(min, std::min(max, value));
    }

    inline cv::Point tupleIntCast(const cv::Point2f &point) {
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
        const float x = center.x + cos(angle) * (radii.width / 2);
        const float y = center.y + sin(angle) * (radii.height / 2);
        return {x, y};
    }



    inline Ellipse growEllipse(const Ellipse &ellipse, const float factor) {
        const cv::Point2f &center = std::get<0>(ellipse);
        const cv::Size2f &radii = std::get<1>(ellipse);
        return std::make_tuple(center, cv::Size2f(radii.width * factor, radii.height * factor), std::get<2>(ellipse));
    }

    inline float getDistance(const cv::Point2f &point1, const cv::Point2f &point2) {
        const float dx = point1.x - point2.x;
        const float dy = point1.y - point2.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    inline float getAngle(const cv::Point2f &point1, const cv::Point2f &point2) {
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

    int getScore(int distance) {
        constexpr int maximumImpactDistance = 48;

        if (distance > maximumImpactDistance) {
            return 0;
        }

        if (distance <= 0) {
            return 570;
        }

        if (distance <= 5) {
            return 570 - (distance * 6);
        }

        return 570 - 30 - ((distance - 5) * 3);
    }

    cv::Rect getCropCoordinates(const cv::Mat &image, int targetZone) {
        int width = image.rows;
        int height = image.cols;
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

        return cv::Rect(y1, x1, y2 - y1, x2 - x1);
    }

    cv::Mat getTargetPicture(const cv::Mat &sheetMat, const int targetZone) {
        const cv::Rect coordinates = getCropCoordinates(sheetMat, targetZone);
        return sheetMat(coordinates).clone();
    }

    std::vector<cv::Point2f> coordinatesToPercentage(const std::vector<cv::Point> &coordinates, const int width, const int height) {
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
        std::cout << "Converted " << percentageCoordinates.size() << " coordinates to percentage." << std::endl;
        return percentageCoordinates;
    }

    std::vector<cv::Point2f> percentageToCoordinates(const std::vector<cv::Point2f> &percentageCoordinates, const int width, const int height) {
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
        return std::vector<Impact>();
    }
}
