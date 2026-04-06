/**
 * @file target_detection.h
 * @brief Target ellipse detection and visualisation functions.
 *
 * Provides functions to detect the concentric ring targets on an
 * underwater shooting sheet, convert coordinates between target-local
 * and sheet-global frames, and draw target overlays on annotated images.
 */

#ifndef SUBVISION_CORE_TARGET_DETECTION_H
#define SUBVISION_CORE_TARGET_DETECTION_H

#include "types.h"
#include <map>
#include <opencv2/opencv.hpp>


namespace subvision {

/**
 * @brief Detect the main target ellipse (contrat ring) in a single target
 * image.
 *
 * Processing pipeline:
 * 1. Convert to XYZ colour space and extract the Z channel
 * 2. Invert and threshold to isolate the dark ring region
 * 3. Remove impact marks to avoid interference
 * 4. Apply heavy morphological operations (erode/dilate)
 * 5. Fit an ellipse and validate its aspect ratio (0.7–1.3)
 * 6. Refine by XOR/AND fallback if initial fit is invalid
 *
 * @param mat BGR image of a single target zone (cropped).
 * @return The detected target Ellipse (center, size, angle).
 *
 * @throws std::runtime_error If no valid ellipse can be detected.
 *
 * @note Performance: typically 10–50 ms per target zone depending
 *       on image complexity and WebAssembly overhead.
 */
Ellipse getTargetEllipse(const cv::Mat &mat);

/**
 * @brief Detect the target ellipse for a specific zone of the sheet.
 *
 * Crops the sheet image to the requested zone using getTargetPicture()
 * and delegates to getTargetEllipse().
 *
 * @param image The full sheet image (standard processing dimensions).
 * @param zone  Zone identifier (SUBVISION_ZONE_*).
 * @return The detected Ellipse in zone-local coordinates.
 *
 * @see getTargetEllipse, getTargetPicture
 */
Ellipse getTargetEllipseForZone(const cv::Mat &image, int zone);

/**
 * @brief Detect target ellipses for all five zones on the sheet.
 *
 * Iterates over all zones (top-left, top-right, bottom-left,
 * bottom-right, center) and calls getTargetEllipseForZone() for each.
 *
 * @param image The full sheet image (standard processing dimensions).
 * @return Map of zone ID → detected Ellipse (in zone-local coordinates).
 *
 * @see getTargetEllipseForZone
 */
std::map<int, Ellipse> getTargetsEllipse(const cv::Mat &image);

/**
 * @brief Convert target-local ellipse coordinates to sheet-global coordinates.
 *
 * Each target zone occupies a quadrant (or center region) of the full sheet.
 * This function offsets each ellipse's center by the zone's position within
 * the full sheet so all ellipses share a common coordinate frame.
 *
 * @param ellipses Map of zone ID → Ellipse in zone-local coordinates.
 * @return Map of zone ID → Ellipse in sheet-global coordinates.
 */
std::map<int, Ellipse>
targetCoordinatesToSheetCoordinates(const std::map<int, Ellipse> &ellipses);

/**
 * @brief Draw target ring overlays on the sheet image.
 *
 * Draws five concentric ellipses for each target (mouche, contrat,
 * petit blanc, moyen blanc, grand blanc) and a crosshair, using
 * red colour with thin lines.
 *
 * @param coordinates Map of zone ID → Ellipse in sheet-global coordinates.
 * @param sheetMat    The sheet image to draw on (modified in place).
 */
void drawTargets(const std::map<int, Ellipse> &coordinates, cv::Mat &sheetMat);

/**
 * @brief Draw the detected sheet boundary on the image.
 *
 * @param sheetMat The image to draw on (modified in place).
 *
 * @note Currently a stub — not yet implemented.
 */
void drawDetectedSheet(cv::Mat &sheetMat);
} // namespace subvision

#endif // SUBVISION_CORE_TARGET_DETECTION_H
