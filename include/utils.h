/**
 * @file utils.h
 * @brief Geometric, mathematical, and coordinate utility functions.
 *
 * Provides helper functions used across the Subvision CV pipeline
 * for angle conversion, point rotation, ellipse geometry, distance
 * and score calculations, and coordinate space transformations.
 */

#ifndef SUBVISION_CORE_UTILS_H
#define SUBVISION_CORE_UTILS_H

#include "types.h"
#include <opencv2/opencv.hpp>


namespace subvision {

/**
 * @brief Convert an angle from degrees to radians.
 *
 * @param angle Angle in degrees.
 * @return Angle in radians.
 */
float toRadians(float angle);

/**
 * @brief Convert an angle from radians to degrees.
 *
 * @param angle Angle in radians.
 * @return Angle in degrees.
 */
float toDegrees(float angle);

/**
 * @brief Cast a floating-point 2D point to an integer 2D point.
 *
 * Truncates (not rounds) each coordinate to int.
 *
 * @param point The source point with float coordinates.
 * @return A cv::Point with integer coordinates.
 */
cv::Point tupleIntCast(const cv::Point2f &point);

/**
 * @brief Rotate a point around a given center by a specified angle.
 *
 * Applies a 2D rotation matrix to translate the point relative to
 * the center, rotate, and translate back.
 *
 * @param center The center of rotation.
 * @param point  The point to rotate.
 * @param angle  Rotation angle in radians.
 * @return The rotated point.
 */
cv::Point2f rotatePoint(const cv::Point2f &center, const cv::Point2f &point,
                        float angle);

/**
 * @brief Compute a point on the perimeter of an ellipse at a given angle.
 *
 * Takes into account the ellipse's center, semi-axes, and rotation angle.
 *
 * @param ellipse The target ellipse (center, size, rotation).
 * @param angle   Angle in degrees at which to sample the ellipse perimeter.
 * @return The 2D point on the ellipse boundary.
 */
cv::Point2f getPointOnEllipse(const Ellipse &ellipse, float angle);

/**
 * @brief Scale an ellipse by a uniform factor around its center.
 *
 * Multiplies both axes of the ellipse by the given factor.
 * A factor of 1.0 returns an identical ellipse; values > 1.0 enlarge,
 * values < 1.0 shrink.
 *
 * @param ellipse The source ellipse.
 * @param factor  Scale factor (e.g., 1.8 = 80% larger).
 * @return A new Ellipse with scaled dimensions.
 */
Ellipse growEllipse(const Ellipse &ellipse, float factor);

/**
 * @brief Compute the Euclidean distance between two 2D points.
 *
 * @param point1 First point.
 * @param point2 Second point.
 * @return Distance in pixels.
 */
float getDistance(const cv::Point2f &point1, const cv::Point2f &point2);

/**
 * @brief Compute the angle (in radians) from point1 to point2.
 *
 * Uses `atan2(dy, dx)` convention.
 *
 * @param point1 Origin point.
 * @param point2 Target point.
 * @return Angle in radians.
 */
float getAngle(const cv::Point2f &point1, const cv::Point2f &point2);

/**
 * @brief Calculate the real-world distance of an impact in millimeters.
 *
 * Maps the pixel distance between the target center and the impact
 * to a real-world distance using the known target radius (45 mm).
 *
 * @param center The target center in pixel coordinates.
 * @param border A reference point on the target border in pixel coordinates.
 * @param impact The detected impact position in pixel coordinates.
 * @return Distance in millimeters (rounded to nearest integer).
 */
int getRealDistance(const cv::Point2f &center, const cv::Point2f &border,
                    const cv::Point2f &impact);

/**
 * @brief Compute the score for a given distance from center.
 *
 * Scoring rules (underwater target shooting federation scale):
 * - Distance > 48 mm: score = 0
 * - Distance <= 0 mm (bullseye): score = 570
 * - Distance 1–5 mm: score = 570 − (distance × 6)
 * - Distance > 5 mm: score = 540 − ((distance − 5) × 3)
 *
 * @param distance Distance from center in millimeters.
 * @return Score value (0–570).
 */
int getScore(int distance);

/**
 * @brief Clamp a float value between a minimum and maximum.
 *
 * @param value The value to clamp.
 * @param min   Minimum allowed value.
 * @param max   Maximum allowed value.
 * @return The clamped value.
 */
float clamp(float value, float min, float max);

/**
 * @brief Get the cropping rectangle for a specific target zone.
 *
 * Divides the sheet image into quadrants (or center region)
 * based on the zone identifier.
 *
 * @param image      The full sheet image (used for dimensions).
 * @param targetZone Zone identifier (SUBVISION_ZONE_*).
 * @return cv::Rect defining the crop region.
 */
cv::Rect getCropCoordinates(const cv::Mat &image, int targetZone);

/**
 * @brief Extract the sub-image for a specific target zone.
 *
 * Crops the sheet image to isolate a single target for
 * per-zone ellipse detection.
 *
 * @param sheetMat   The full sheet image.
 * @param targetZone Zone identifier (SUBVISION_ZONE_*).
 * @return A cloned cv::Mat of the cropped target region.
 */
cv::Mat getTargetPicture(const cv::Mat &sheetMat, int targetZone);

/**
 * @brief Convert absolute pixel coordinates to percentage-based coordinates.
 *
 * Normalises coordinates to [0, 1] range relative to the given dimensions.
 * Used to store sheet corner positions in a resolution-independent format.
 *
 * @param coordinates Vector of integer coordinates.
 * @param width       Reference width for normalisation.
 * @param height      Reference height for normalisation.
 * @return Vector of normalised floating-point coordinates.
 */
std::vector<cv::Point2f>
coordinatesToPercentage(const std::vector<cv::Point> &coordinates, int width,
                        int height);

/**
 * @brief Convert percentage-based coordinates back to absolute pixel
 * coordinates.
 *
 * Inverse of coordinatesToPercentage().
 *
 * @param percentageCoordinates Normalised coordinates in [0, 1] range.
 * @param width                 Target width for denormalisation.
 * @param height                Target height for denormalisation.
 * @return Vector of absolute pixel coordinates.
 */
std::vector<cv::Point2f>
percentageToCoordinates(const std::vector<cv::Point2f> &percentageCoordinates,
                        int width, int height);

/**
 * @brief Create an empty vector of Impact objects.
 *
 * Convenience factory function, primarily used by test code.
 *
 * @return An empty std::vector<Impact>.
 */
std::vector<Impact> createImpactVector();
} // namespace subvision

#endif // SUBVISION_CORE_UTILS_H
