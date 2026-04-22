/**
 * @file sheet_detection.h
 * @brief Shooting sheet detection and perspective correction.
 *
 * Provides functions to automatically detect the shooting sheet
 * (plastron) in a photograph, extract its corner coordinates, and
 * apply a perspective transform to produce a standardised flat image.
 */

#ifndef SHEET_DETECTION_H
#define SHEET_DETECTION_H
#include <opencv2/core/types.hpp>

namespace subvision {

/**
 * @brief Automatically detect and extract the shooting sheet from an image.
 *
 * Combines getSheetCoordinates() and getSheetPictureManually() to perform
 * full automatic sheet extraction in a single call.
 *
 * @param image Input BGR image containing the shooting sheet.
 * @return Perspective-corrected sheet image at standard processing dimensions
 *         (PICTURE_WIDTH_SHEET_DETECTION × PICTURE_HEIGHT_SHEET_DETECTION).
 *
 * @throws std::runtime_error If no valid sheet contour is found.
 *
 * @see getSheetCoordinates, getSheetPictureManually
 */
cv::Mat getSheetPicture(const cv::Mat &image);

/**
 * @brief Extract the shooting sheet using pre-computed corner coordinates.
 *
 * Applies a perspective transform using the provided corner points
 * (in normalised percentage format) to produce a flat, standardised
 * sheet image.
 *
 * @param image       Input BGR image containing the shooting sheet.
 * @param coordinates Four corner points in normalised [0, 1] coordinates.
 *                    Must contain exactly 4 points in the order:
 *                    top-left, top-right, bottom-right, bottom-left.
 * @return Perspective-corrected sheet image at standard dimensions.
 *
 * @throws std::runtime_error If coordinates are empty or not exactly 4 points.
 */
cv::Mat getSheetPictureManually(const cv::Mat &image,
                                const std::vector<cv::Point2f> coordinates);

/**
 * @brief Detect the four corner coordinates of the shooting sheet.
 *
 * Processing pipeline:
 * 1. Resize to standard dimensions
 * 2. Convert to HLS and extract the lightness channel
 * 3. Threshold to find bright regions (the white sheet)
 * 4. Find contours and select the biggest valid quadrilateral
 * 5. Return corners as normalised percentage coordinates
 *
 * @param sheet_mat Input BGR image containing the shooting sheet.
 * @return Vector of 4 corner points in normalised [0, 1] coordinates.
 *
 * @throws std::runtime_error If no valid quadrilateral contour is found.
 *
 * @note The returned coordinates are resolution-independent percentages,
 *       suitable for storage and later reuse with getSheetPictureManually().
 */
std::vector<cv::Point2f> getSheetCoordinates(const cv::Mat &sheet_mat);
/**
 * @brief Detect the four corner coordinates of the shooting sheet using AruCo markers.
 *
 *
 * @param sheet_mat Input BGR image containing the shooting sheet.
 * @return Vector of 4 corner points in normalised [0, 1] coordinates.
 *
 * @throws std::runtime_error If no valid quadrilateral contour is found.
 *
 * @note The returned coordinates are resolution-independent percentages,
 *       suitable for storage and later reuse with getSheetPictureManually().
 */
std::vector<cv::Point2f> getSheetCoordinatesUsingAruCoMarkers(const cv::Mat &sheet_mat);
} // namespace subvision

#endif // SHEET_DETECTION_H
