#ifndef SUBVISION_CV_IMAGE_PROCESSING_H
#define SUBVISION_CV_IMAGE_PROCESSING_H

#include <opencv2/opencv.hpp>
#include <vector>
#include "types.h"

namespace subvision {
    // Obtenir le plus grand contour valide
    std::vector<cv::Point> getBiggestValidContour(const std::vector<std::vector<cv::Point>> &contours);

    // Obtenir le masque des impacts
    cv::Mat getImpactsMask(const cv::Mat &image);

    // Obtenir les coordonnées des impacts
    std::vector<cv::Point2f> getImpactsCoordinates(const cv::Mat &image);

    // Obtenir un masque de couleur
    cv::Mat getColorMask(const cv::Mat &mat, const cv::Scalar &color);

    // Extraire une ellipse d'une image
    Ellipse retrieveEllipse(const cv::Mat &image);
}

#endif //SUBVISION_CV_IMAGE_PROCESSING_H
