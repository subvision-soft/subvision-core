#ifndef SUBVISION_CV_TARGET_DETECTION_H
#define SUBVISION_CV_TARGET_DETECTION_H

#include <opencv2/opencv.hpp>
#include <map>
#include "types.h"

namespace subvision {
    // Obtenir l'ellipse cible
    Ellipse getTargetEllipse(const cv::Mat &mat);

    // Obtenir l'ellipse cible pour une zone
    Ellipse getTargetEllipseForZone(const cv::Mat &image, int zone);

    // Obtenir les ellipses pour toutes les cibles
    std::map<int, Ellipse> getTargetsEllipse(const cv::Mat &image);

    // Convertir les coordonnées de cible en coordonnées de feuille
    std::map<int, Ellipse> targetCoordinatesToSheetCoordinates(const std::map<int, Ellipse> &ellipses);

    // Dessiner les cibles sur l'image
    void drawTargets(const std::map<int, Ellipse> &coordinates, cv::Mat &sheetMat);
}

#endif //SUBVISION_CV_TARGET_DETECTION_H
