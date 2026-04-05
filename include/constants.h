/**
 * @file constants.h
 * @brief Global constants for the Subvision CV library.
 *
 * Defines target zone identifiers, standard image processing dimensions,
 * and morphological kernels used throughout the detection pipeline.
 */

#ifndef SUBVISION_CORE_CONSTANTS_H
#define SUBVISION_CORE_CONSTANTS_H

#include <opencv2/opencv.hpp>

namespace subvision {

/** @name Target Zone Identifiers
 *  Constants identifying the five target zones on a standard
 *  underwater shooting sheet (four corners + center).
 *  @{
 */
const int SUBVISION_ZONE_TOP_LEFT = 0;     ///< Top-left target zone.
const int SUBVISION_ZONE_TOP_RIGHT = 1;    ///< Top-right target zone.
const int SUBVISION_ZONE_BOTTOM_LEFT = 2;  ///< Bottom-left target zone.
const int SUBVISION_ZONE_BOTTOM_RIGHT = 3; ///< Bottom-right target zone.
const int SUBVISION_ZONE_CENTER = 4;       ///< Center target zone.
const int SUBVISION_ZONE_UNDEFINED = -1;   ///< Undefined or unresolved zone.
/** @} */

/** @name Image Processing Dimensions
 *  Standard dimensions to which sheet images are resized
 *  before detection processing. All coordinate calculations
 *  are performed relative to these dimensions.
 *  @{
 */
const int PICTURE_WIDTH_SHEET_DETECTION =
    2000; ///< Standard processing width in pixels.
const int PICTURE_HEIGHT_SHEET_DETECTION =
    2000; ///< Standard processing height in pixels.
/** @} */

/**
 * @brief Kernel size for morphological operations, derived from processing
 * dimensions.
 *
 * Set to 1/200th of the standard processing width (10×10 pixels at 2000px).
 */
const cv::Size KERNEL_SIZE(PICTURE_WIDTH_SHEET_DETECTION / 200,
                           PICTURE_WIDTH_SHEET_DETECTION / 200);

/**
 * @brief Pre-built elliptical structuring element for morphological operations.
 *
 * Used in contour filtering, mask cleanup, and noise removal stages
 * of the image processing pipeline.
 */
const cv::Mat ROUND_KERNEL =
    cv::getStructuringElement(cv::MORPH_ELLIPSE, KERNEL_SIZE);
} // namespace subvision

#endif // SUBVISION_CORE_CONSTANTS_H
