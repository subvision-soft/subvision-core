#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <gtest/gtest.h>
#include "../include/constants.h"
#include "../include/image_processing.h"
#include "../include/target_detection.h"

namespace fs = std::filesystem;

const std::string TESTS_RESOURCES_PATH = (fs::current_path() / "resources").string();

class EllipseDetectionTests : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    void runEllipsesTest(const std::string& folder) {
        std::string imgPath = TESTS_RESOURCES_PATH + "/" + folder + "/cropped_sheet.jpg";
        std::string expectedMaskPath = TESTS_RESOURCES_PATH + "/" + folder + "/expected_visuals.jpg";

        cv::Mat img = cv::imread(imgPath);
        cv::resize(img,img, cv::Size(subvision::PICTURE_WIDTH_SHEET_DETECTION, subvision::PICTURE_HEIGHT_SHEET_DETECTION));
        std::map<int, subvision::Ellipse> ellipses = subvision::getTargetsEllipse(img);
        std::map<int, subvision::Ellipse> targetsEllipsis = subvision::targetCoordinatesToSheetCoordinates(ellipses);

        cv::Mat blackMat = cv::Mat::zeros(subvision::PICTURE_HEIGHT_SHEET_DETECTION, subvision::PICTURE_WIDTH_SHEET_DETECTION, CV_8UC1);
        for (const auto& pair : targetsEllipsis) {
            const auto& value = pair.second;
            cv::Point center = cv::Point(static_cast<int>(std::get<0>(value).x), static_cast<int>(std::get<0>(value).y));
            cv::Size size = cv::Size(static_cast<int>(std::get<1>(value).width), static_cast<int>(std::get<1>(value).height));
            float angle = std::get<2>(value);
            cv::ellipse(blackMat, center, cv::Size(size.width/2, size.height/2), angle, 0, 360, 255, -1);
        }

        cv::Mat expectedMask = cv::imread(expectedMaskPath, cv::IMREAD_GRAYSCALE);
        cv::resize(expectedMask,expectedMask, cv::Size(subvision::PICTURE_WIDTH_SHEET_DETECTION, subvision::PICTURE_HEIGHT_SHEET_DETECTION));
        cv::Mat binaryExpectedMask;
        cv::threshold(expectedMask, binaryExpectedMask, 127, 255, cv::THRESH_BINARY);

        cv::Mat xorMat;
        cv::bitwise_xor(blackMat, binaryExpectedMask, xorMat);
        double similarity = 1.0 - static_cast<double>(cv::countNonZero(xorMat)) / xorMat.total();

        ASSERT_GE(similarity, 0.995) << "Ellipses detection failed for folder " << folder << ", similarity: " << similarity;
    }
};

TEST_F(EllipseDetectionTests, TestEllipsesDetection) {
    for (const auto& entry : fs::directory_iterator(TESTS_RESOURCES_PATH)) {
        if (entry.is_directory()) {
            std::string folder = entry.path().filename().string();
            if (folder != "TODO" && folder.find("WIP") == std::string::npos) {
                SCOPED_TRACE("Testing folder: " + folder);
                runEllipsesTest(folder);
            }
        }
    }
}
