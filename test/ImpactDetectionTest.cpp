#include <filesystem>
#include <numeric>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <gtest/gtest.h>
#include "../include/constants.h"
#include "../include/image_processing.h"
#include "../include/utils.h"
#include "../include/logging.h"

namespace fs = std::filesystem;

const std::string TESTS_RESOURCES_PATH = (fs::current_path() / "resources").string();

class ImpactDetectionTests : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    std::vector<cv::Point2f> getMaskCenters(const cv::Mat& mask) {
        std::vector<cv::Point2f> centers;
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        for (const auto& contour : contours) {
            if (contour.size() >= 5) {
                cv::RotatedRect ellipse = cv::fitEllipse(contour);
                centers.push_back(ellipse.center);
            } else if (!contour.empty()) {
                auto m = cv::moments(contour);
                if (m.m00 != 0) {
                    centers.push_back(cv::Point2f(static_cast<float>(m.m10 / m.m00), static_cast<float>(m.m01 / m.m00)));
                } else {
                    centers.push_back(contour[0]);
                }
            }
        }
        return centers;
    }

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



        fs::path debugDir = fs::path(TESTS_RESOURCES_PATH) / folder / "debug";
        fs::create_directories(debugDir);
        cv::imwrite((debugDir / "mask_expected.png").string(), binaryExpectedMask);
        cv::imwrite((debugDir / "mask_detected.png").string(), maskImpacts);
        cv::imwrite((debugDir / "xor.png").string(), xorMat);
        cv::imwrite((debugDir / "input.png").string(), img);


        ASSERT_GE(similarity, 0.999) << "Impacts mask failed for folder " << folder << ", similarity: " << similarity;
        // Extract expected centers directly from the binary mask instead of simulating BGR
        std::vector<cv::Point2f> realCoordinates = getMaskCenters(binaryExpectedMask);
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
    subvision::log("Looking for resources in: " + TESTS_RESOURCES_PATH);
    subvision::log("Current path: " + fs::current_path().string());

    if (!fs::exists(TESTS_RESOURCES_PATH)) {
        subvision::log("Resources directory does not exist!");
    }
    
    int pictureCount = 0;
    for (const auto& entry : fs::directory_iterator(TESTS_RESOURCES_PATH)) {
        if (entry.is_directory()) {
            std::string folder = entry.path().filename().string();
            if (folder != "TODO" && folder.find("WIP") == std::string::npos && folder == "10") {
                SCOPED_TRACE("Testing folder: " + folder);
                runImpactsTest(folder);
                pictureCount++;
            }
        }
    }
    subvision::log("Impact Detection: Tested " + std::to_string(pictureCount) + " pictures");
}
