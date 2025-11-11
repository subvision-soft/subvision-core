#include "../include/impact_detection.h"

#include "sheet_detection.h"
#include "../include/constants.h"
#include "../include/utils.h"
#include "../include/image_processing.h"
#include "../include/target_detection.h"

namespace subvision {

    std::vector<Impact> drawAndGetImpactsPoints(const std::vector<cv::Point2f> &impacts, cv::Mat &sheetMat,
                                              const std::map<int, Ellipse> &targetsEllipsis) {
        const auto start = std::chrono::high_resolution_clock::now();
        std::vector<Impact> points;
        points.reserve(impacts.size());

        const cv::Scalar blue(255, 0, 0);
        const cv::Scalar black(0, 0, 0);
        const cv::Scalar orange(0, 165, 255);
        const cv::Scalar white(255, 255, 255);
        constexpr int perpendicularLineLength = 25;
        constexpr float pi = 3.14159265f;

        for (const auto &impact: impacts) {
            int closestZone = SUBVISION_ZONE_UNDEFINED;
            float minDistanceSq = std::numeric_limits<float>::max();

            for (const auto &[zone, ellipse]: targetsEllipsis) {
                const cv::Point2f &ellipseCenter = std::get<0>(ellipse);
                const float dx = impact.x - ellipseCenter.x;
                const float dy = impact.y - ellipseCenter.y;
                const float distanceSq = dx * dx + dy * dy;

                if (distanceSq < minDistanceSq) {
                    minDistanceSq = distanceSq;
                    closestZone = zone;
                }
            }

            const Ellipse targetEllipsis = growEllipse(targetsEllipsis.at(closestZone), 1.8f);
            const cv::Point center = tupleIntCast(std::get<0>(targetEllipsis));
            const float radAngle = getAngle(impact, center) + pi;
            const cv::Point2f pointOnEllipse = getPointOnEllipse(targetEllipsis, radAngle);
            const cv::Point pointOnEllipseInt = tupleIntCast(pointOnEllipse);
            const cv::Point impactInt = tupleIntCast(impact);

            line(sheetMat, center, pointOnEllipseInt, black, 2);
            circle(sheetMat, center, 5, blue, -1);
            circle(sheetMat, pointOnEllipseInt, 5, blue, -1);
            circle(sheetMat, impactInt, 5, orange, -1);

            const float dx = pointOnEllipse.x - center.x;
            const float dy = pointOnEllipse.y - center.y;
            const float invNorm = 1.0f / std::sqrt(dx * dx + dy * dy);
            const float perpDx = -dy * invNorm * perpendicularLineLength;
            const float perpDy = dx * invNorm * perpendicularLineLength;

            const cv::Point perpPoint1(static_cast<int>(impact.x + perpDx), static_cast<int>(impact.y + perpDy));
            const cv::Point perpPoint2(static_cast<int>(impact.x - perpDx), static_cast<int>(impact.y - perpDy));
            line(sheetMat, perpPoint1, perpPoint2, orange, 2);

            const int realDistance = getRealDistance(center, pointOnEllipse, impact);
            const int score = getScore(realDistance);

            const std::string scoreStr = std::to_string(score);
            putText(sheetMat, scoreStr, impactInt, cv::FONT_HERSHEY_SIMPLEX, 2, black, 20);
            putText(sheetMat, scoreStr, impactInt, cv::FONT_HERSHEY_SIMPLEX, 2, white, 10);

            points.emplace_back(realDistance, score, closestZone, toDegrees(radAngle) + 180.0f, 1);
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour drawAndGetImpactsPoints: " << elapsed.count() << " secondes" << std::endl;
        return points;
    }

    bool retrieveImpacts(const cv::Mat &imageToProcess, ImpactResults &results) {

        cv::Mat sheetMat = getSheetPicture(imageToProcess.clone());

        // Resize to standard dimensions if needed
        if (sheetMat.cols != PICTURE_WIDTH_SHEET_DETECTION || sheetMat.rows != PICTURE_HEIGHT_SHEET_DETECTION) {
            resize(sheetMat, sheetMat, cv::Size(PICTURE_WIDTH_SHEET_DETECTION, PICTURE_HEIGHT_SHEET_DETECTION));
        }

        // Get targets ellipses
        std::map<int, Ellipse> targetsEllipsis = getTargetsEllipse(sheetMat);
        targetsEllipsis = targetCoordinatesToSheetCoordinates(targetsEllipsis);

        // Get impacts coordinates
        const std::vector<cv::Point2f> impactsCoordinates = getImpactsCoordinates(sheetMat);

        // Draw targets
        drawTargets(targetsEllipsis, sheetMat);

        // Draw impacts and get points
        const std::vector<Impact> points = drawAndGetImpactsPoints(impactsCoordinates, sheetMat, targetsEllipsis);

        // Set results
        results.annotatedImage = sheetMat; // Assign the encoded string
        results.impacts = points;

        return true;
    }
}
