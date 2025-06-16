#include <iostream>
#include <fstream>
#include <vector>

#include "include/encoding.h"
#include "include/impact_detection.h"
#include "include/types.h"
#include <opencv2/opencv.hpp>

#include "constants.h"
#include "image_processing.h"
#include "sheet_detection.h"
#include "target_detection.h"

int main() {
    std::ifstream file("C:\\Users\\paulc\\CLionProjects\\subvision-core\\demo_sheet.txt");
    std::vector<unsigned char> data;
    if (file) {
        std::string line, number;
        while (std::getline(file, line, ',')) {
            int value = std::stoi(line);
            if (value < 0 || value > 255) {
                std::cerr << "Valeur hors limites: " << value << std::endl;
                continue;
            }
            data.push_back(static_cast<unsigned char>(value));
        }
    } else {
        std::cerr << "Impossible d'ouvrir le fichier demo_sheet.txt" << std::endl;
    }



    // cv::Mat image_to_process = cv::imread("C:\\\\Users\\\\Paul\\\\CLionProjects\\\\subvision-cv\\\\sheet.png");

    cv::Mat image_to_process(4000,1800 , CV_8UC4, data.data());
    if (image_to_process.empty()) {
        std::cerr << "Error: Could not open or find the image!" << std::endl;
        return -1;
    }
    cv::cvtColor(image_to_process, image_to_process, cv::COLOR_RGBA2BGR);

    auto sheet_picture = subvision::getSheetPicture(image_to_process);
    // auto aspectRatio = static_cast<float>(image_to_process.cols) / static_cast<float>(image_to_process.rows);
    // if (aspectRatio < 1.0f) {
    //     // Si l'image est plus haute que large, on la redimensionne en gardant le ratio
    //     int newHeight = subvision::PICTURE_HEIGHT_SHEET_DETECTION;
    //     int newWidth = static_cast<int>(newHeight * aspectRatio);
    //     cv::resize(image_to_process, image_to_process, cv::Size(newWidth, newHeight));
    // } else {
    //     // Si l'image est plus large que haute, on la redimensionne en gardant le ratio
    //     int newWidth = subvision::PICTURE_WIDTH_SHEET_DETECTION;
    //     int newHeight = static_cast<int>(newWidth / aspectRatio);
    //     cv::resize(image_to_process, image_to_process, cv::Size(newWidth, newHeight));
    // }
    // if (image_to_process.empty()) {
    //     std::cerr << "Error: Could not open or find the image!" << std::endl;
    //     return -1;
    // }
    // std::vector<cv::Vec3f> circles = {};
    //
    // cv::Mat impacts = subvision::getImpactsMask(image_to_process);
    // cv::Mat notImpacts;
    // bitwise_not(impacts, notImpacts);
    // bitwise_and(image_to_process, image_to_process, image_to_process, notImpacts);
    //
    //
    // int minRadius;
    // int maxRadius;
    // int min_dist;
    // if (aspectRatio < 1.0f) {
    //     // Si l'image est plus haute que large, on utilise la hauteur pour les rayons
    //     minRadius = image_to_process.cols / 20.0;
    //     maxRadius = image_to_process.cols / 5.0;
    //     min_dist = image_to_process.cols / 6;
    // } else {
    //     // Si l'image est plus large que haute, on utilise la largeur pour les rayons
    //     minRadius = image_to_process.rows / 20.0;
    //     maxRadius = image_to_process.rows / 5.0;
    //     min_dist = image_to_process.rows / 6.0;
    // }
    //
    // auto copySheetMat = image_to_process.clone();
    // cv::Mat hls;
    // cv::cvtColor(copySheetMat, hls, cv::COLOR_BGR2HLS);
    // cv::Mat hlsChannels[3];
    // cv::split(hls, hlsChannels);
    // cv::Mat lightness = hlsChannels[1];
    // cv::Mat saturated = hlsChannels[2];
    //
    // cv::Mat cannyEdges;
    // cv::Canny(lightness, cannyEdges, 50, 150, 3);
    //
    // double minVal, maxVal;
    // cv::minMaxLoc(lightness, &minVal, &maxVal);
    //
    // maxVal = std::max(maxVal, 120.0);
    // minVal = (maxVal - minVal) / 2 + minVal;
    //
    // cv::inRange(lightness, cv::Scalar(minVal), cv::Scalar(maxVal), lightness);
    //
    // std::cout << "minVal: " << minVal << ", maxVal: " << maxVal << std::endl;
    // std::cout << "minRadius: " << minRadius << ", maxRadius: " << maxRadius << ", min_dist: " << min_dist << std::endl;
    //
    // cv::HoughCircles(lightness, circles, cv::HOUGH_GRADIENT, 1, min_dist, 100, 30, minRadius, maxRadius);
    // for (const auto &circle: circles) {
    //     cv::Point center(static_cast<int>(circle[0]), static_cast<int>(circle[1]));
    //     int radius = static_cast<int>(circle[2]);
    //     cv::Scalar color(0, 255, 0);
    //     cv::circle(image_to_process, center, radius, color, 2);
    //     cv::circle(image_to_process, center, 2, color, -1);
    // }
    //
    //
    //
    // cv::resize(image_to_process, image_to_process, cv::Size(400, 400));
    cv::imshow("test", sheet_picture);
    cv::waitKey(0);




    return 0;
}
