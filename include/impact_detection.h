#ifndef SUBVISION_CORE_IMPACT_DETECTION_H
#define SUBVISION_CORE_IMPACT_DETECTION_H

#include <opencv2/opencv.hpp>
#include <vector>
#include "types.h"

namespace subvision {
    // Dessiner les impacts sur l'image et obtenir les points d'impact
    std::vector<Impact> drawAndGetImpactsPoints(const std::vector<cv::Point2f> &impacts, cv::Mat &sheetMat,
                                                const std::map<int, Ellipse> &targetsEllipsis);

    // Traiter une image pour détecter les impacts
    bool retrieveImpacts(const cv::Mat &imageToProcess, ImpactResults &results);
}

#endif //SUBVISION_CORE_IMPACT_DETECTION_H
