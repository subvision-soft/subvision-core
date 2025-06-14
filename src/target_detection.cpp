#include "../include/target_detection.h"
#include "../include/constants.h"
#include "../include/utils.h"
#include "../include/image_processing.h"

namespace subvision {

    Ellipse getTargetEllipse(const cv::Mat &mat) {
        auto start = std::chrono::high_resolution_clock::now();

        // Créer un masque circulaire centré
        cv::Mat circle = cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);
        cv::circle(circle, cv::Point(mat.cols / 2, mat.rows / 2), static_cast<int>(mat.cols / 2.2), cv::Scalar(255), -1);

        // Conversion en espace de couleur XYZ et extraction du canal Z
        cv::Mat xyz;
        cvtColor(mat, xyz, cv::COLOR_BGR2XYZ);
        std::vector<cv::Mat> xyzChannels;
        split(xyz, xyzChannels);
        cv::Mat value = xyzChannels[2];

        // Inversion et seuillage adaptatif
        bitwise_not(value, value);
        double minVal, maxVal;
        minMaxLoc(value, &minVal, &maxVal);
        minVal = maxVal - (maxVal - minVal) / 1.5;
        cv::Mat valueMask;
        inRange(value, cv::Scalar(minVal), cv::Scalar(maxVal), valueMask);

        // Suppression des impacts détectés
        cv::Mat impacts = getImpactsMask(mat);
        cv::Mat notImpacts;
        bitwise_not(impacts, notImpacts);
        bitwise_and(valueMask, notImpacts, valueMask);

        // Morphologie pour fermer les trous
        cv::Mat close;
        cv::erode(valueMask, close, cv::Mat(), cv::Point(-1, -1), 10);
        cv::dilate(close, close, cv::Mat(), cv::Point(-1, -1), 20);
        cv::erode(close, close, cv::Mat(), cv::Point(-1, -1), 10);

        Ellipse ellipse = retrieveEllipse(close);

        auto ellipseIsValid = [](const Ellipse& e) {
            float w = std::get<1>(e).width;
            float h = std::get<1>(e).height;
            return w >= h * 0.7f && w <= h * 1.3f;
        };

        try {
            // Générer un masque d'ellipse et fusionner avec le masque détecté
            cv::Mat empty = cv::Mat::zeros(mat.size(), mat.type());
            cv::Point center = tupleIntCast(std::get<0>(ellipse));
            cv::Size size(static_cast<int>(std::get<1>(ellipse).width), static_cast<int>(std::get<1>(ellipse).height));
            float angle = std::get<2>(ellipse);

            std::vector<cv::Point> ellipsePoints;
            ellipse2Poly(center, cv::Size2f(size.width / 2, size.height / 2), static_cast<int>(angle), 0, 360, 1, ellipsePoints);
            fillConvexPoly(empty, ellipsePoints, cv::Scalar(255, 255, 255));

            cv::Mat emptyGray;
            cvtColor(empty, emptyGray, cv::COLOR_BGR2GRAY);

            cv::Mat xor_;
            bitwise_xor(emptyGray, close, xor_);
            bitwise_or(close, xor_, close);

            ellipse = retrieveEllipse(close);

            if (!ellipseIsValid(ellipse)) {
                throw std::runtime_error("Problem during visual detection 1");
            }
        } catch (const std::exception &) {
            // Si l'ellipse n'est pas valide, restreindre le masque et réessayer
            cv::Mat empty = cv::Mat::zeros(mat.size(), mat.type());
            cv::Point center = tupleIntCast(std::get<0>(ellipse));
            cv::Size size(static_cast<int>(std::get<1>(ellipse).width), static_cast<int>(std::get<1>(ellipse).height));
            float angle = std::get<2>(ellipse);

            std::vector<cv::Point> ellipsePoints;
            ellipse2Poly(center, cv::Size2f(size.width / 2, size.height / 2), static_cast<int>(angle), 0, 360, 1, ellipsePoints);
            fillConvexPoly(empty, ellipsePoints, cv::Scalar(255, 255, 255));

            cv::Mat emptyGray;
            cvtColor(empty, emptyGray, cv::COLOR_BGR2GRAY);

            bitwise_and(close, emptyGray, close);

            ellipse = retrieveEllipse(close);

            if (!ellipseIsValid(ellipse)) {
                throw std::runtime_error("Problem during visual detection");
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Temps écoulé pour getTargetEllipse: " << elapsed.count() << " secondes" << std::endl;
        return ellipse;
    }

    Ellipse getTargetEllipseForZone(const cv::Mat &image, int zone) {
        return getTargetEllipse(getTargetPicture(image, zone));
    }

    std::map<int, Ellipse> getTargetsEllipse(const cv::Mat &image) {
        std::vector<int> zones = {
            SUBVISION_ZONE_TOP_LEFT, SUBVISION_ZONE_TOP_RIGHT, SUBVISION_ZONE_CENTER,
            SUBVISION_ZONE_BOTTOM_LEFT, SUBVISION_ZONE_BOTTOM_RIGHT
        };

        std::map<int, Ellipse> ellipses;

        for (const auto &zone: zones) {
            ellipses[zone] = getTargetEllipseForZone(image, zone);
        }

        return ellipses;
    }

    std::map<int, Ellipse> targetCoordinatesToSheetCoordinates(const std::map<int, Ellipse> &ellipses) {
        std::map<int, Ellipse> newEllipses;

        for (const auto &pair: ellipses) {
            const auto &key = pair.first;
            const auto &value = pair.second;
            cv::Point2f center = std::get<0>(value);
            cv::Size2f size = std::get<1>(value);
            float angle = std::get<2>(value);

            if (key == SUBVISION_ZONE_TOP_LEFT) {
                newEllipses[key] = std::make_tuple(center, size, angle);
            } else if (key == SUBVISION_ZONE_BOTTOM_LEFT) {
                newEllipses[key] = std::make_tuple(
                    cv::Point2f(center.x, center.y + PICTURE_HEIGHT_SHEET_DETECTION / 2),
                    size, angle
                );
            } else if (key == SUBVISION_ZONE_TOP_RIGHT) {
                newEllipses[key] = std::make_tuple(
                    cv::Point2f(center.x + PICTURE_WIDTH_SHEET_DETECTION / 2, center.y),
                    size, angle
                );
            } else if (key == SUBVISION_ZONE_BOTTOM_RIGHT) {
                newEllipses[key] = std::make_tuple(
                    cv::Point2f(center.x + PICTURE_WIDTH_SHEET_DETECTION / 2,
                            center.y + PICTURE_HEIGHT_SHEET_DETECTION / 2),
                    size, angle
                );
            } else if (key == SUBVISION_ZONE_CENTER) {
                newEllipses[key] = std::make_tuple(
                    cv::Point2f(center.x + PICTURE_WIDTH_SHEET_DETECTION / 4,
                            center.y + PICTURE_HEIGHT_SHEET_DETECTION / 4),
                    size, angle
                );
            }
        }

        return newEllipses;
    }

    void drawTargets(const std::map<int, Ellipse> &coordinates, cv::Mat &sheetMat) {
        for (const auto &pair: coordinates) {
            const auto &key = pair.first;
            const auto &ellipseContrat = pair.second;
            int drawingWidth = 1;
            cv::Scalar targetColor(0, 0, 255);

            Ellipse ellipseCrossTip = growEllipse(ellipseContrat, 2.2f);
            Ellipse ellipseMouche = growEllipse(ellipseContrat, 0.2f);
            Ellipse ellipsePetitBlanc = growEllipse(ellipseContrat, 0.6f);
            Ellipse ellipseMoyenBlanc = growEllipse(ellipseContrat, 1.4f);
            Ellipse ellipseGrandBlanc = growEllipse(ellipseContrat, 1.8f);

            cv::Point center = tupleIntCast(std::get<0>(ellipseContrat));
            cv::Size size = cv::Size(static_cast<int>(std::get<1>(ellipseContrat).width),
                                static_cast<int>(std::get<1>(ellipseContrat).height));
            float angle = std::get<2>(ellipseContrat);

            ellipse(sheetMat, center, cv::Size2f(size.width / 2, size.height / 2), angle, 0, 360, targetColor,
                    drawingWidth);

            cv::Point centerMouche = tupleIntCast(std::get<0>(ellipseMouche));
            cv::Size sizeMouche = cv::Size(static_cast<int>(std::get<1>(ellipseMouche).width),
                                    static_cast<int>(std::get<1>(ellipseMouche).height));
            float angleMouche = std::get<2>(ellipseMouche);

            ellipse(sheetMat, centerMouche, cv::Size2f(sizeMouche.width / 2, sizeMouche.height / 2),
                    angleMouche, 0, 360, targetColor, drawingWidth);

            cv::Point centerPetitBlanc = tupleIntCast(std::get<0>(ellipsePetitBlanc));
            cv::Size sizePetitBlanc = cv::Size(static_cast<int>(std::get<1>(ellipsePetitBlanc).width),
                                        static_cast<int>(std::get<1>(ellipsePetitBlanc).height));
            float anglePetitBlanc = std::get<2>(ellipsePetitBlanc);

            ellipse(sheetMat, centerPetitBlanc, cv::Size2f(sizePetitBlanc.width / 2, sizePetitBlanc.height / 2),
                    anglePetitBlanc, 0, 360, targetColor, drawingWidth);

            cv::Point centerMoyenBlanc = tupleIntCast(std::get<0>(ellipseMoyenBlanc));
            cv::Size sizeMoyenBlanc = cv::Size(static_cast<int>(std::get<1>(ellipseMoyenBlanc).width),
                                        static_cast<int>(std::get<1>(ellipseMoyenBlanc).height));
            float angleMoyenBlanc = std::get<2>(ellipseMoyenBlanc);

            ellipse(sheetMat, centerMoyenBlanc, cv::Size2f(sizeMoyenBlanc.width / 2, sizeMoyenBlanc.height / 2),
                    angleMoyenBlanc, 0, 360, targetColor, drawingWidth);

            cv::Point centerGrandBlanc = tupleIntCast(std::get<0>(ellipseGrandBlanc));
            cv::Size sizeGrandBlanc = cv::Size(static_cast<int>(std::get<1>(ellipseGrandBlanc).width),
                                        static_cast<int>(std::get<1>(ellipseGrandBlanc).height));
            float angleGrandBlanc = std::get<2>(ellipseGrandBlanc);

            ellipse(sheetMat, centerGrandBlanc, cv::Size2f(sizeGrandBlanc.width / 2, sizeGrandBlanc.height / 2),
                    angleGrandBlanc, 0, 360, targetColor, drawingWidth);

            cv::Point2f topPoint = getPointOnEllipse(ellipseCrossTip, toRadians(90.0f));
            cv::Point2f bottomPoint = getPointOnEllipse(ellipseCrossTip, toRadians(270.0f));
            cv::Point2f leftPoint = getPointOnEllipse(ellipseCrossTip, toRadians(180.0f));
            cv::Point2f rightPoint = getPointOnEllipse(ellipseCrossTip, toRadians(0.0f));

            line(sheetMat, tupleIntCast(topPoint), tupleIntCast(bottomPoint), targetColor, drawingWidth);
            line(sheetMat, tupleIntCast(leftPoint), tupleIntCast(rightPoint), targetColor, drawingWidth);
        }
    }
}
