#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <gtest/gtest.h>
#include "../include/constants.h"
#include "../include/image_processing.h"
#include "../include/target_detection.h"
#include "../include/utils.h"

namespace fs = std::filesystem;

const std::string TESTS_RESOURCES_PATH = (fs::current_path() / "resources").string();

class ImpactDetectionTests : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    void runImpactsTest(const std::string& folder) {
        std::string imgPath = TESTS_RESOURCES_PATH + "/" + folder + "/cropped_sheet.jpg";
        std::string maskPath = TESTS_RESOURCES_PATH + "/" + folder + "/expected_impacts.jpg";

        cv::Mat img = cv::imread(imgPath);
        cv::resize(img,img, cv::Size(subvision::PICTURE_WIDTH_SHEET_DETECTION, subvision::PICTURE_HEIGHT_SHEET_DETECTION));
        std::vector<cv::Point2f> impacts = subvision::getImpactsCoordinates(img);

        cv::Mat expectedMask = cv::imread(maskPath, cv::IMREAD_GRAYSCALE);
        cv::resize(expectedMask,expectedMask, cv::Size(subvision::PICTURE_WIDTH_SHEET_DETECTION, subvision::PICTURE_HEIGHT_SHEET_DETECTION));
        cv::Mat binaryExpectedMask;
        cv::threshold(expectedMask, binaryExpectedMask, 127, 255, cv::THRESH_BINARY);

        cv::Mat maskImpacts = subvision::getImpactsMask(img);
        cv::Mat xorMat;
        cv::bitwise_xor(maskImpacts, binaryExpectedMask, xorMat);
        double similarity = 1.0 - static_cast<double>(cv::countNonZero(xorMat)) / xorMat.total();

        ASSERT_GE(similarity, 0.999) << "Impacts mask failed for folder " << folder << ", similarity: " << similarity;

        std::vector<cv::Mat> splitResult;
        cv::split(binaryExpectedMask, splitResult);
        cv::Mat hsvSimulate;
        cv::merge(std::vector{splitResult[0], splitResult[0], splitResult[0]}, hsvSimulate);
        cv::Mat bgrSimulate;
        cv::cvtColor(hsvSimulate, bgrSimulate, cv::COLOR_HSV2BGR);
        std::vector<cv::Point2f> realCoordinates = subvision::getImpactsCoordinates(bgrSimulate);
        ASSERT_EQ(impacts.size(), realCoordinates.size()) << "Impacts detection failed for folder " << folder << ", impacts count: " << impacts.size();

        std::vector<double> distances;
        for (const auto& real : realCoordinates) {
            double minDist = std::numeric_limits<double>::max();
            for (const auto& impact : impacts) {
                double dist = subvision::getDistance(real, impact);
                minDist = std::min(minDist, dist);
            }
            distances.push_back(minDist);
        }

        double averageDistance = 0.0;
        if (!distances.empty()) {
            averageDistance = std::accumulate(distances.begin(), distances.end(), 0.0) / distances.size();
        }
        ASSERT_LE(averageDistance, 4.0) << "Impacts coordinates failed for folder " << folder << ", avg distance: " << averageDistance;
    }
};

TEST_F(ImpactDetectionTests, TestImpactsDetection) {
    std::cout << "Looking for resources in: " << TESTS_RESOURCES_PATH << std::endl;
    std::cout << "Current path: " << fs::current_path() << std::endl;
    
    if (!fs::exists(TESTS_RESOURCES_PATH)) {
        std::cout << "Resources directory does not exist!" << std::endl;
    }
    
    int pictureCount = 0;
    for (const auto& entry : fs::directory_iterator(TESTS_RESOURCES_PATH)) {
        if (entry.is_directory()) {
            std::string folder = entry.path().filename().string();
            if (folder != "TODO" && folder.find("WIP") == std::string::npos) {
                SCOPED_TRACE("Testing folder: " + folder);
                runImpactsTest(folder);
                pictureCount++;
            }
        }
    }
    std::cout << "Impact Detection: Tested " << pictureCount << " pictures" << std::endl;
}
