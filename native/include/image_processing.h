/**
 * @file image_processing.h
 * @brief Low-level image processing functions for the Subvision CV pipeline.
 *
 * Provides contour analysis, impact mask generation, impact coordinate
 * extraction, colour masking, and ellipse fitting routines used by
 * higher-level detection modules.
 */

#ifndef SUBVISION_CORE_IMAGE_PROCESSING_H
#define SUBVISION_CORE_IMAGE_PROCESSING_H

#include "types.h"
#include <opencv2/opencv.hpp>
#include <vector>


namespace subvision {

/**
 * @brief Find the largest valid quadrilateral contour from a list of contours.
 *
 * Iterates through all contours, approximates each to a polygon,
 * and selects the largest 4-sided polygon whose area ratio is between
 * 10% and 90% of the total image area, and whose corner angles are
 * all within 70°–110° (approximately rectangular).
 *
 * Used primarily to detect the shooting sheet boundary.
 *
 * @param contours Vector of contour point vectors to search.
 * @return The 4-point approximation of the biggest valid contour,
 *         or an empty vector if none qualifies.
 *
 * @note All area/angle calculations use the standard processing
 *       dimensions defined in constants.h.
 */
std::vector<cv::Point>
getBiggestValidContour(const std::vector<std::vector<cv::Point>> &contours);

/**
 * @brief Generate a binary mask highlighting detected impact locations.
 *
 * Processing pipeline:
 * 1. Convert to Lab and apply CLAHE on the L channel for normalisation
 * 2. Convert to HSV and threshold for red hue ranges (impacts are red)
 * 3. Apply morphological open/close to remove noise
 * 4. Filter contours by area (0.005%–1% of image) and circularity (>0.6)
 *
 * @param image Input BGR image (typically a cropped sheet image).
 * @return Binary mask (CV_8UC1) where white pixels indicate impact regions.
 *
 * @warning Input must be in BGR colour space.
 */
cv::Mat getImpactsMask(const cv::Mat &image);

/**
 * @brief Detect impact center coordinates from an image.
 *
 * Generates the impact mask via getImpactsMask(), finds external contours,
 * fits ellipses to each contour (minimum 5 points), and returns the
 * centers of all valid ellipses.
 *
 * @param image Input BGR image of the sheet.
 * @return Vector of 2D center points of detected impacts.
 *
 * @see getImpactsMask
 */
std::vector<cv::Point2f> getImpactsCoordinates(const cv::Mat &image);

/**
 * @brief Create a binary mask for a specific colour in the image.
 *
 * Converts the target colour to HSV, constructs a narrow hue range
 * (±10), and thresholds the input image in HSV space. The resulting
 * mask is cleaned with erosion and dilation.
 *
 * @param mat   Input BGR image.
 * @param color Target colour as an RGB cv::Scalar.
 * @return Binary mask (CV_8UC1) of matching regions.
 */
cv::Mat getColorMask(const cv::Mat &mat, const cv::Scalar &color);

/**
 * @brief Extract the best-fit ellipse from a binary image.
 *
 * Finds external contours, selects the one with the largest area,
 * and fits an ellipse using `cv::fitEllipse()`. Falls back to
 * using non-zero pixel points if the contour has fewer than 5 points.
 *
 * @param image Binary input image (CV_8UC1).
 * @return The detected Ellipse (center, size, angle), or a zero-sized
 *         ellipse at origin if no valid contour is found.
 */
Ellipse retrieveEllipse(const cv::Mat &image);
} // namespace subvision

#endif // SUBVISION_CORE_IMAGE_PROCESSING_H
