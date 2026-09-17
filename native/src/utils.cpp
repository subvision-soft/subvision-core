#include <cmath>
#include <vector>
#include <algorithm>
#include <ranges>

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
        for (const int i: std::views::iota(1, distance)) {
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

    static void fillShortestPath(std::vector<bool> &arr) {
        std::vector<size_t> true_indices;

        for (size_t i = 0; i < arr.size(); ++i) {
            if (arr[i])
                true_indices.push_back(i);
        }

        if (true_indices.empty())
            return;

        const size_t first_idx = true_indices.front();
        const size_t last_idx = true_indices.back();

        if (const size_t distance = last_idx - first_idx; distance > arr.size() / 2) {
            for (size_t i = last_idx; i < arr.size(); ++i)
                arr[i] = true;

            for (size_t i = 0; i <= first_idx; ++i)
                arr[i] = true;

            for (size_t i = first_idx + 1; i < last_idx; ++i)
                arr[i] = false;
        } else {
            for (size_t i = first_idx; i <= last_idx; ++i)
                arr[i] = true;
        }
    }
    std::vector<cv::Point> cleanupEllipticalContour(const std::vector<cv::Point> &contour) {
        const cv::Moments M = cv::moments(contour);


        const double cx = M.m10 / M.m00;
        const double cy = M.m01 / M.m00;


        const size_t n = contour.size();

        std::vector<double> distances(n);

        for (size_t i = 0; i < n; ++i)
        {
            const double dx = contour[i].x - cx;
            const double dy = contour[i].y - cy;

            distances[i] = std::sqrt(dx * dx + dy * dy);
        }

        std::vector bad_jumps(n - 1, false);

        for (size_t i = 0; i < n - 1; ++i)
        {
            const double diff = std::abs(distances[i + 1] - distances[i]);

            bad_jumps[i] = diff > JUMP_THRESHOLD;
        }
        fillShortestPath(bad_jumps);


        std::vector<cv::Point> cleaned_contour;
        for (size_t i = 0; i < contour.size() - 1; ++i)
        {
            if (!bad_jumps[i])
            {
                cleaned_contour.push_back(contour[i]);
            }
        }
        return cleaned_contour;
    }

}
