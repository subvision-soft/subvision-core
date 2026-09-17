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
#include <list>
#include <tuple>
#include <utility>

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
    struct AreaSpecs;

    enum class Federation {
        UNKNOWN = -1,
        CMAS = 0,
        FFESSM = 1
    };

    enum class Event {
        UNKNOWN = -1,
        PRECISION = 0,
        BIATHLON = 1,
        SUPER_BIATHLON = 2,
        RELAY = 3
    };

    struct TargetSpecs {
        std::list<AreaSpecs> areas;
        int maxScore;
        explicit TargetSpecs(const std::list<AreaSpecs> &r, int max)
            : areas(r), maxScore(max) {
        }
    };


    struct AreaSpecs {
        int increment;
        int radius;
        bool main;
        bool ring;

        AreaSpecs() = default;

        explicit AreaSpecs(const int inc, const int rad, const bool isMain, const bool isRing = true)
            : increment(inc),  radius(rad), main(isMain), ring(isRing) {
        }
    };

    struct TargetSheetSpecs {
        Federation federation;
        std::list<Event> event;
        TargetSpecs targetSpecs;
        int numberOfTargets;
        /**
         * @brief Constructs a TargetSheetSpecs with all fields.
         *
         * @param fed        The federation to which the target sheet belongs.
         * @param evt        The events associated with the target sheet.
         * @param specs      The target specifications.
         * @param numTargets The number of targets on the sheet.
         */
        explicit TargetSheetSpecs(const Federation fed, const std::list<Event> &evt, TargetSpecs specs,
                                  const int numTargets)
            : federation(fed), event(evt), targetSpecs(std::move(specs)), numberOfTargets(numTargets) {
        }
    };

    /**
     * @brief A vector of all supported target sheet specifications.
     */
    const inline std::vector TARGET_SHEET_SPECS = {
        {
            TargetSheetSpecs(
                Federation::CMAS,
                {Event::PRECISION, Event::BIATHLON, Event::SUPER_BIATHLON},
                TargetSpecs({
                    AreaSpecs(10,  6, false),
                    AreaSpecs(10,  16, false),
                    AreaSpecs(5,  26, true),
                    AreaSpecs(5,  36, false),
                    AreaSpecs(5,  46, false),
                    AreaSpecs(5,  56, false)
                },460),
                5
            ),
            TargetSheetSpecs(
                Federation::CMAS,
                {Event::RELAY},
                TargetSpecs({
                    AreaSpecs(10,  6, false),
                    AreaSpecs(10,  16, false),
                    AreaSpecs(5,  26, true),
                    AreaSpecs(5,  36, false),
                },460),
                9
            ),
            TargetSheetSpecs(
                Federation::FFESSM,
                {Event::PRECISION, Event::BIATHLON, Event::SUPER_BIATHLON, Event::RELAY},
                TargetSpecs({
                    AreaSpecs(6, 5, false),
                    AreaSpecs(3, 15, false),
                    AreaSpecs(3, 25, true),
                    AreaSpecs(3, 35, false),
                    AreaSpecs(3, 45, false),
                    AreaSpecs(3, 48, false,false),
                },570),
                5
            )

        }
    };


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
        int distance; ///< Distance from center in millimeters.
        int score; ///< Computed score based on distance (0–570 scale).
        int zone; ///< Target zone identifier (see constants.h for SUBVISION_ZONE_* values).
        float angle; ///< Angular position in degrees relative to target center.
        int count; ///< Number of impacts at this location (usually 1).

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
            : distance(distance), score(score), zone(zone), angle(angle), count(count) {
        }
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
        cv::Mat annotatedImage; ///< Image with detected targets and impacts drawn on it.
        std::vector<Impact> impacts; ///< List of detected impacts with scores.
    };
}

#endif //SUBVISION_CORE_TYPES_H
