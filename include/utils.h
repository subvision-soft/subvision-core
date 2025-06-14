#ifndef SUBVISION_CV_UTILS_H
#define SUBVISION_CV_UTILS_H

#include <opencv2/opencv.hpp>
#include "types.h"

namespace subvision {
    // Conversion d'angle de degrés en radians
    float toRadians(float angle);

    // Conversion d'angle de radians en degrés
    float toDegrees(float angle);

    // Conversion d'un point flottant en point entier
    cv::Point tupleIntCast(const cv::Point2f &point);

    // Rotation d'un point autour d'un centre
    cv::Point2f rotatePoint(const cv::Point2f &center, const cv::Point2f &point, float angle);

    // Obtenir un point sur une ellipse à un angle donné
    cv::Point2f getPointOnEllipse(const Ellipse &ellipse, float angle);

    // Agrandir une ellipse par un facteur
    Ellipse growEllipse(const Ellipse &ellipse, float factor);

    // Calculer la distance entre deux points
    float getDistance(const cv::Point2f &point1, const cv::Point2f &point2);

    // Calculer l'angle entre deux points
    float getAngle(const cv::Point2f &point1, const cv::Point2f &point2);

    // Calculer la distance réelle en mm
    int getRealDistance(const cv::Point2f &center, const cv::Point2f &border, const cv::Point2f &impact);

    // Calculer le score en fonction de la distance
    int getScore(int distance);

    // Clamper une valeur entre un minimum et un maximum
    float clamp(float value, float min, float max);

    // Obtenir les coordonnées de recadrage pour une zone
    cv::Rect getCropCoordinates(const cv::Mat &image, int targetZone);

    // Obtenir l'image pour une zone cible
    cv::Mat getTargetPicture(const cv::Mat &sheetMat, int targetZone);

    // Convertir des coordonnées en pourcentage
    std::vector<cv::Point2f> coordinatesToPercentage(const std::vector<cv::Point> &coordinates, int width, int height);

    // Convertir des pourcentages en coordonnées
    std::vector<cv::Point> percentageToCoordinates(const std::vector<cv::Point2f> &percentageCoordinates, int width, int height);

    // Créer un vecteur d'impacts vide
    std::vector<Impact> createImpactVector();
}

#endif //SUBVISION_CV_UTILS_H
