#include "../include/utils.h"
#include "../include/constants.h"

namespace subvision {

    float toRadians(float angle) {
        return angle * static_cast<float>(CV_PI) / 180.0f;
    }

    float toDegrees(float angle) {
        return angle * 180.0f / static_cast<float>(CV_PI);
    }

    float clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }

    cv::Point tupleIntCast(const cv::Point2f &point) {
        return cv::Point(static_cast<int>(point.x), static_cast<int>(point.y));
    }

    cv::Point2f rotatePoint(const cv::Point2f &center, const cv::Point2f &point, float angle) {
        float s = sin(angle);
        float c = cos(angle);

        float translatedX = point.x - center.x;
        float translatedY = point.y - center.y;

        float rotatedX = translatedX * c - translatedY * s;
        float rotatedY = translatedX * s + translatedY * c;

        return cv::Point2f(rotatedX + center.x, rotatedY + center.y);
    }

    cv::Point2f getPointOnEllipse(const Ellipse &ellipse, float angle) {
        const cv::Point2f &center = std::get<0>(ellipse);
        const cv::Size2f &radii = std::get<1>(ellipse);
        float x = center.x + cos(angle) * (radii.width / 2);
        float y = center.y + sin(angle) * (radii.height / 2);
        return cv::Point2f(x, y);
    }



    Ellipse growEllipse(const Ellipse &ellipse, float factor) {
        const cv::Point2f &center = std::get<0>(ellipse);
        const cv::Size2f &radii = std::get<1>(ellipse);
        return std::make_tuple(center, cv::Size2f(radii.width * factor, radii.height * factor), std::get<2>(ellipse));
    }

    float getDistance(const cv::Point2f &point1, const cv::Point2f &point2) {
        return sqrt(pow(point1.x - point2.x, 2) + pow(point1.y - point2.y, 2));
    }

    float getAngle(const cv::Point2f &point1, const cv::Point2f &point2) {
        return atan2(point2.y - point1.y, point2.x - point1.x);
    }

    int getRealDistance(const cv::Point2f &center, const cv::Point2f &border, const cv::Point2f &impact) {
        float length = getDistance(center, border);
        float distance = getDistance(center, impact);
        float percent = distance / length;
        float realLength = 45.0f;
        float millimeterDistance = realLength * percent;
        return cvRound(millimeterDistance);
    }

    int getScore(int distance) {
        int score = 570;
        int i = 0;
        int maximumImpactDistance = 48;

        if (distance > maximumImpactDistance) {
            return 0;
        }

        for (i = 0; i < 5; i++) {
            if (distance <= 0) {
                break;
            }
            score -= 6;
            distance -= 1;
        }

        for (; i < 48; i++) {
            if (distance <= 0) {
                break;
            }
            score -= 3;
            distance -= 1;
        }

        return score;
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

    cv::Mat getTargetPicture(const cv::Mat &sheetMat, int targetZone) {
        cv::Mat sheetMatClone = sheetMat.clone();
        cv::Rect coordinates = getCropCoordinates(sheetMatClone, targetZone);
        return sheetMatClone(coordinates);
    }

    std::vector<cv::Point2f> coordinatesToPercentage(const std::vector<cv::Point> &coordinates, int width, int height) {
        std::vector<cv::Point2f> percentageCoordinates;
        for (const auto &coordinate: coordinates) {
            percentageCoordinates.push_back(cv::Point2f(
                static_cast<float>(coordinate.x) / width,
                static_cast<float>(coordinate.y) / height
            ));
        }
        std::cout << "Converted " << percentageCoordinates.size() << " coordinates to percentage." << std::endl;
        return percentageCoordinates;
    }

    std::vector<cv::Point2f> percentageToCoordinates(const std::vector<cv::Point2f> &percentageCoordinates, int width, int height) {
        std::vector<cv::Point2f> coordinates;
        for (const auto &percentageCoordinate: percentageCoordinates) {
            coordinates.push_back(cv::Point(
                static_cast<int>(percentageCoordinate.x * width),
                static_cast<int>(percentageCoordinate.y * height)
            ));
        }
        return coordinates;
    }

    std::vector<Impact> createImpactVector() {
        return std::vector<Impact>();
    }
}
