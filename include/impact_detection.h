/**
 * @file impact_detection.h
 * @brief High-level impact detection and scoring pipeline.
 *
 * Provides the main entry point for processing a target shooting sheet image:
 * detecting sheet boundaries, locating targets, finding impacts, and computing
 * scores.
 */

#ifndef SUBVISION_CORE_IMPACT_DETECTION_H
#define SUBVISION_CORE_IMPACT_DETECTION_H

#include "types.h"
#include <opencv2/opencv.hpp>
#include <vector>


namespace subvision {

/**
 * @brief Annotate impacts on the sheet image and compute their scores.
 *
 * For each detected impact point:
 * 1. Determine the closest target zone
 * 2. Calculate the real-world distance from the target center
 * 3. Compute the score using the federation scoring table
 * 4. Draw visual annotations (lines, circles, score text) on the image
 *
 * @param impacts          Vector of detected impact center coordinates
 * (sheet-global).
 * @param sheetMat         The sheet image to annotate (modified in place).
 * @param targetsEllipsis  Map of zone ID → target Ellipse in sheet-global
 * coordinates.
 * @return Vector of Impact objects with distance, score, zone, and angle.
 *
 * @see retrieveImpacts for the full processing pipeline
 *
 * @code
 * auto impacts = subvision::getImpactsCoordinates(sheetMat);
 * auto targets = subvision::getTargetsEllipse(sheetMat);
 * targets = subvision::targetCoordinatesToSheetCoordinates(targets);
 * auto results = subvision::drawAndGetImpactsPoints(impacts, sheetMat,
 * targets);
 * @endcode
 */
std::vector<Impact>
drawAndGetImpactsPoints(const std::vector<cv::Point2f> &impacts,
                        cv::Mat &sheetMat,
                        const std::map<int, Ellipse> &targetsEllipsis);

/**
 * @brief Process a raw image to detect and score all impacts.
 *
 * This is the main entry point of the Subvision CV pipeline. It performs:
 * 1. Sheet detection (automatic or manual via provided coordinates)
 * 2. Perspective correction to standard dimensions (2000×2000)
 * 3. Target ellipse detection for all five zones
 * 4. Impact localisation via red-colour analysis
 * 5. Score computation and image annotation
 *
 * @param imageToProcess The input image in BGR format.
 * @param results        Output structure receiving the annotated image and
 * impact list.
 * @param coordinates    Optional pre-computed sheet corner coordinates
 * (percentage-based). If empty, automatic sheet detection is performed.
 * @return `true` if processing succeeded, `false` on failure (e.g., sheet not
 * found).
 *
 * @note In WebAssembly, input images arrive as RGBA and are converted to BGR
 *       before calling this function.
 * @warning The input image is cloned internally; the original is not modified.
 *
 * Example (C++):
 * @code
 * cv::Mat image = cv::imread("target_sheet.jpg");
 * subvision::ImpactResults results;
 * bool ok = subvision::retrieveImpacts(image, results);
 * if (ok) {
 *     for (const auto& impact : results.impacts) {
 *         std::cout << "Score: " << impact.score << std::endl;
 *     }
 * }
 * @endcode
 */
bool retrieveImpacts(const cv::Mat &imageToProcess, ImpactResults &results,
                     const std::vector<cv::Point2f> &coordinates = {});
} // namespace subvision

#endif // SUBVISION_CORE_IMPACT_DETECTION_H
