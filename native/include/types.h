/**
 * @file types.h
 * @brief Core data types used throughout the Subvision CV library.
 *
 * Defines the fundamental structures for representing impacts,
 * impact analysis results, and geometric primitives used in
 * underwater target shooting sheet analysis.
 */

#ifndef SUBVISION_CORE_TYPES_H
#define SUBVISION_CORE_TYPES_H

#include <opencv2/opencv.hpp>
#include <tuple>

/**
 * @namespace subvision
 * @brief Root namespace for the Subvision computer vision library.
 *
 * Contains all classes, functions, and types used for underwater
 * target shooting sheet detection, impact localization, and scoring.
 * The library supports native C++, WebAssembly (via Emscripten),
 * and .NET (via C++/CLI) platforms.
 */
namespace subvision {

    /**
     * @brief Geometric ellipse representation as a tuple of center, size, and rotation angle.
     *
     * Components:
     * - `cv::Point2f` — center coordinates of the ellipse
     * - `cv::Size2f` — width and height (full axes, not semi-axes)
     * - `float` — rotation angle in degrees
     *
     * Used to represent detected target rings on the shooting sheet.
     */
    using Ellipse = std::tuple<cv::Point2f, cv::Size2f, float>;

    /**
     * @brief Represents a single detected impact (shot) on a target.
     *
     * Each impact contains its distance from the target center,
     * the computed score, the zone it belongs to, and its angular
     * position relative to the target center.
     */
    struct Impact {
        int distance;   ///< Distance from center in millimeters.
        int score;      ///< Computed score based on distance (0–570 scale).
        int zone;       ///< Target zone identifier (see constants.h for SUBVISION_ZONE_* values).
        float angle;    ///< Angular position in degrees relative to target center.
        int count;      ///< Number of impacts at this location (usually 1).

        /**
         * @brief Constructs an Impact with all fields.
         *
         * @param distance Distance from the target center in millimeters.
         * @param score    Computed score for this impact.
         * @param zone     Target zone identifier.
         * @param angle    Angular position in degrees.
         * @param count    Number of impacts (typically 1).
         */
        Impact(int distance, int score, int zone, float angle, int count)
            : distance(distance), score(score), zone(zone), angle(angle), count(count) {}
    };

    /**
     * @brief Container for the results of impact detection processing.
     *
     * Holds the annotated image (with targets and impacts drawn)
     * and the list of detected impacts with their scores.
     *
     * @note The annotated image is in BGR format (OpenCV default).
     *       Platform-specific wrappers convert to RGBA before returning.
     */
    struct ImpactResults {
        cv::Mat annotatedImage;          ///< Image with detected targets and impacts drawn on it.
        std::vector<Impact> impacts;     ///< List of detected impacts with scores.
    };
}

#endif //SUBVISION_CORE_TYPES_H
