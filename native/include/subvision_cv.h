/**
 * @file subvision_cv.h
 * @brief Umbrella header for the Subvision CV library.
 *
 * Include this single header to access the entire Subvision CV public API.
 * It aggregates all module headers: constants, types, utilities, image
 * processing, target detection, impact detection, and sheet detection.
 *
 * @code
 * #include "subvision_cv.h"
 *
 * cv::Mat image = cv::imread("sheet.jpg");
 * subvision::ImpactResults results;
 * if (subvision::retrieveImpacts(image, results)) {
 *     for (const auto& impact : results.impacts) {
 *         std::cout << "Score: " << impact.score << std::endl;
 *     }
 * }
 * @endcode
 */

#ifndef SUBVISION_CORE_H
#define SUBVISION_CORE_H

#include "constants.h"
#include "image_processing.h"
#include "impact_detection.h"
#include "sheet_detection.h"
#include "target_detection.h"
#include "types.h"
#include "utils.h"


#endif // SUBVISION_CORE_H
