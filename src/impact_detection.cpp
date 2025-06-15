#include "../include/impact_detection.h"

#include "sheet_detection.h"
#include "../include/constants.h"
#include "../include/utils.h"
#include "../include/image_processing.h"
#include "../include/target_detection.h"
#include "../include/encoding.h"

namespace subvision {

    std::vector<Impact> drawAndGetImpactsPoints(const std::vector<cv::Point2f> &impacts, cv::Mat &sheetMat,
                                              const std::map<int, Ellipse> &targetsEllipsis) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<Impact> points;
        cv::Scalar blue(255, 0, 0), black(0, 0, 0), orange(0, 165, 255), white(255, 255, 255);
        int perpendicularLineLength = 25;

        for (const auto &impact: impacts) {
            // Find closest zone
            int closestZone = SUBVISION_ZONE_UNDEFINED;
            float minDistance = std::numeric_limits<float>::max();
            for (const auto &pair: targetsEllipsis) {
                const auto &zone = pair.first;
                const auto &ellipse = pair.second;
                float distance = getDistance(impact, std::get<0>(ellipse));
                if (distance < minDistance) {
                    minDistance = distance;
                    closestZone = zone;
                }
            }

            Ellipse targetEllipsis = growEllipse(targetsEllipsis.at(closestZone), 1.8f);
            cv::Point center = tupleIntCast(std::get<0>(targetEllipsis));
            float radAngle = getAngle(impact, center) + toRadians(180.0f);
            cv::Point2f pointOnEllipse = getPointOnEllipse(targetEllipsis, radAngle);

            line(sheetMat, center, tupleIntCast(pointOnEllipse), black, 2);
            circle(sheetMat, center, 5, blue, -1);
            circle(sheetMat, tupleIntCast(pointOnEllipse), 5, blue, -1);
            circle(sheetMat, tupleIntCast(impact), 5, orange, -1);

            float dx = pointOnEllipse.x - center.x;
            float dy = pointOnEllipse.y - center.y;
            float norm = sqrt(dx * dx + dy * dy);
            float perpDx = -dy / norm * perpendicularLineLength;
            float perpDy = dx / norm * perpendicularLineLength;

            cv::Point perpPoint1(static_cast<int>(impact.x + perpDx), static_cast<int>(impact.y + perpDy));
            cv::Point perpPoint2(static_cast<int>(impact.x - perpDx), static_cast<int>(impact.y - perpDy));
            line(sheetMat, perpPoint1, perpPoint2, orange, 2);

            int realDistance = getRealDistance(center, pointOnEllipse, impact);
            int score = getScore(realDistance);

            putText(sheetMat, std::to_string(score), tupleIntCast(impact), cv::FONT_HERSHEY_SIMPLEX, 2, black, 20);
            putText(sheetMat, std::to_string(score), tupleIntCast(impact), cv::FONT_HERSHEY_SIMPLEX, 2, white, 10);

            points.push_back(Impact(realDistance, score, closestZone, toDegrees(radAngle) + 180.0f, 1));
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
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
        std::vector<cv::Point2f> impactsCoordinates = getImpactsCoordinates(sheetMat);

        // Draw targets
        drawTargets(targetsEllipsis, sheetMat);

        // Draw impacts and get points
        std::vector<Impact> points = drawAndGetImpactsPoints(impactsCoordinates, sheetMat, targetsEllipsis);

        // Set results
        results.annotatedImage = sheetMat; // Assign the encoded string
        results.impacts = points;

        return true;
    }
}
