#include "../include/image_processing.h"

#include "../include/constants.h"
#include "../include/logging.h"
#include "../include/utils.h"


namespace subvision {
std::vector<cv::Point>
getBiggestValidContour(const std::vector<std::vector<cv::Point>> &contours) {
  log("Start processing getBiggestValidContour with " +
      std::to_string(contours.size()) + " contours");
  std::vector<cv::Point> biggestContour;
  double biggestArea = 0;
  constexpr double totalArea =
      PICTURE_WIDTH_SHEET_DETECTION * PICTURE_HEIGHT_SHEET_DETECTION;
  constexpr double minAreaRatio = 0.1;
  constexpr double maxAreaRatio = 0.9;
  constexpr float minAngle = 70.0f;
  constexpr float maxAngle = 110.0f;
  constexpr float invPI180 = 180.0f / static_cast<float>(CV_PI);

  std::vector<cv::Point> approx;
  approx.reserve(4);

  for (const auto &contour : contours) {
    log("Processing contour with size: " + std::to_string(contour.size()));
    if (contour.size() < 4)
      continue;

    const double epsilon = 0.01 * cv::arcLength(contour, true);
    approx.clear();
    cv::approxPolyDP(contour, approx, epsilon, true);

    if (approx.size() != 4)
      continue;

    const double area = cv::contourArea(approx);
    if (area <= biggestArea)
      continue;

    const double areaRatio = area / totalArea;
    if (areaRatio < minAreaRatio || areaRatio > maxAreaRatio)
      continue;

    bool validAngles = true;
    for (int i = 0; i < 4; ++i) {
      const cv::Point &p1 = approx[i];
      const cv::Point &p2 = approx[(i + 1) % 4];
      const cv::Point &p3 = approx[(i + 2) % 4];

      const float dx1 = static_cast<float>(p1.x - p2.x);
      const float dy1 = static_cast<float>(p1.y - p2.y);
      const float dx2 = static_cast<float>(p3.x - p2.x);
      const float dy2 = static_cast<float>(p3.y - p2.y);

      const float dot = dx1 * dx2 + dy1 * dy2;
      const float mag1Sq = dx1 * dx1 + dy1 * dy1;
      const float mag2Sq = dx2 * dx2 + dy2 * dy2;

      if (mag1Sq < 1e-12f || mag2Sq < 1e-12f) {
        validAngles = false;
        break;
      }

      const float invMag = 1.0f / std::sqrt(mag1Sq * mag2Sq);
      const float cosAngle = clamp(dot * invMag, -1.0f, 1.0f);
      const float angle = std::acos(cosAngle) * invPI180;

      if (angle < minAngle || angle > maxAngle) {
        validAngles = false;
        break;
      }
    }

    if (!validAngles)
      continue;

    biggestContour = approx;
    biggestArea = area;
  }

  log("Biggest contour found with size: " +
      std::to_string(biggestContour.size()));
  return biggestContour;
}

/**
 * @brief Generate a red-colour ratio mask from a BGR image.
 *
 * Computes the red channel ratio R/(R+G+B) and combines it with
 * HSV saturation thresholding to isolate strongly red regions.
 * This is an internal helper not exposed in the public API.
 *
 * @param image Input BGR image.
 * @return Binary mask (CV_8U) where white pixels indicate red regions.
 */
cv::Mat redRatioMask(const cv::Mat &image) {

  // --- Convert to float ---
  cv::Mat imgFloat;
  image.convertTo(imgFloat, CV_32FC3);

  std::vector<cv::Mat> bgr;
  cv::split(imgFloat, bgr);

  cv::Mat B = bgr[0];
  cv::Mat G = bgr[1];
  cv::Mat R = bgr[2];

  // --- Red ratio ---
  cv::Mat sum = R + G + B;
  cv::Mat redRatio;
  cv::divide(R, sum + 1e-6, redRatio); // safe divide

  // --- Saturation from HSV ---
  cv::Mat hsv;
  cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

  std::vector<cv::Mat> hsvChannels;
  cv::split(hsv, hsvChannels);
  cv::Mat saturation = hsvChannels[1]; // 0–255

  // --- Thresholds ---
  float redThreshold = 0.6f; // tweak this
  int satThreshold = 80;     // tweak this

  cv::Mat redMask = redRatio > redThreshold;
  cv::Mat satMask = saturation > satThreshold;

  cv::Mat finalMask = redMask & satMask;

  finalMask.convertTo(finalMask, CV_8U, 255);
  return finalMask;
}
cv::Mat getImpactsMask(const cv::Mat &image) {
  cv::Mat normalized;
  cv::Mat lab;
  cv::cvtColor(image, lab, cv::COLOR_BGR2Lab);

  std::vector<cv::Mat> labChannels;
  cv::split(lab, labChannels);

  // CLAHE on L channel
  cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
  clahe->apply(labChannels[0], labChannels[0]);

  cv::merge(labChannels, lab);
  // cv::imshow("lab", lab);
  cv::cvtColor(lab, normalized, cv::COLOR_Lab2BGR);
  // cv::imshow("normalized", normalized);
  cv::Mat hsv;
  cv::cvtColor(normalized, hsv, cv::COLOR_BGR2HSV);

  // cv::imshow("hsv", hsv);

  cv::Mat mask1, mask2, redMask;

  // lower red
  cv::inRange(hsv, cv::Scalar(0, 80, 80), cv::Scalar(10, 255, 255), mask1);

  // cv::imshow("mask1", mask1);

  // upper red
  cv::inRange(hsv, cv::Scalar(170, 80, 80), cv::Scalar(180, 255, 255), mask2);

  // cv::imshow("mask2", mask2);

  cv::bitwise_or(mask1, mask2, redMask);

  // cv::imshow("redMask", redMask);
  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, {5, 5});
  cv::morphologyEx(redMask, redMask, cv::MORPH_OPEN, kernel);
  cv::morphologyEx(redMask, redMask, cv::MORPH_CLOSE, kernel);
  std::vector<std::vector<cv::Point>> contours;
  // cv::imshow("redMaskMorph", redMask);
  cv::findContours(redMask, contours, cv::RETR_EXTERNAL,
                   cv::CHAIN_APPROX_SIMPLE);

  cv::Mat result = cv::Mat::zeros(redMask.size(), CV_8UC1);

  const double minArea = image.cols * image.rows * 0.00005;
  const double maxArea = image.cols * image.rows * 0.01;

  for (const auto &contour : contours) {
    double area = cv::contourArea(contour);
    if (area < minArea || area > maxArea)
      continue;

    double perimeter = cv::arcLength(contour, true);
    if (perimeter == 0)
      continue;

    double circularity = 4 * CV_PI * area / (perimeter * perimeter);

    if (circularity < 0.6) // keep round shapes
      continue;

    cv::drawContours(result, std::vector<std::vector<cv::Point>>{contour}, -1,
                     cv::Scalar(255), cv::FILLED);
  }

  return result;
}

std::vector<cv::Point2f> getImpactsCoordinates(const cv::Mat &image) {
  log("getImpactsCoordinates");
  const auto start = std::chrono::high_resolution_clock::now();
  const cv::Mat mask = getImpactsMask(image);
  std::vector<std::vector<cv::Point>> contours;
  findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

  std::vector<cv::Point2f> centers;
  centers.reserve(contours.size());

  for (const auto &contour : contours) {
    if (contour.size() >= 5) {
      const cv::RotatedRect ellipse = fitEllipse(contour);
      if (!isnan(ellipse.center.x) && !isnan(ellipse.center.y)) {
        centers.push_back(ellipse.center);
      }
    }
  }
  const auto end = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> elapsed = end - start;
  log("Temps écoulé pour getImpactsCoordinates: " +
      std::to_string(elapsed.count()) + " secondes");
  return centers;
}

cv::Mat getColorMask(const cv::Mat &mat, const cv::Scalar &color) {
  log("getColorMask");
  const cv::Mat colorMat(1, 1, CV_8UC3, color);
  cv::Mat hsv;
  cvtColor(colorMat, hsv, cv::COLOR_RGB2HSV);

  const cv::Scalar minVal(hsv.at<cv::Vec3b>(0, 0)[0] - 10, 100, 50);
  const cv::Scalar maxVal(hsv.at<cv::Vec3b>(0, 0)[0] + 10, 255, 255);

  cv::Mat hsvMat;
  cvtColor(mat, hsvMat, cv::COLOR_BGR2HSV);

  cv::Mat mask;
  inRange(hsvMat, minVal, maxVal, mask);

  cv::Mat kernel = getStructuringElement(
      cv::MORPH_RECT, cv::Size(PICTURE_WIDTH_SHEET_DETECTION / 200,
                               PICTURE_HEIGHT_SHEET_DETECTION / 200));
  cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
  cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
  threshold(mask, mask, 127, 255, cv::THRESH_BINARY);

  return mask;
}

Ellipse retrieveEllipse(const cv::Mat &image) {
  log("retrieveEllipse");
  const auto start = std::chrono::high_resolution_clock::now();
  std::vector<std::vector<cv::Point>> contours;
  findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

  Ellipse emptyEllipse =
      std::make_tuple(cv::Point2f(0, 0), cv::Size2f(0, 0), 0.0f);

  if (contours.empty()) {
    return emptyEllipse;
  }

  const auto maxIt = std::max_element(
      contours.begin(), contours.end(),
      [](const std::vector<cv::Point> &a, const std::vector<cv::Point> &b) {
        return contourArea(a) < contourArea(b);
      });

  const std::vector<cv::Point> &biggestContour = *maxIt;

  if (biggestContour.size() >= 5) {
    const cv::RotatedRect rotatedRect = fitEllipse(biggestContour);
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    log("Temps écoulé pour retrieveEllipse: " +
        std::to_string(elapsed.count()) + " secondes");
    return std::make_tuple(rotatedRect.center, rotatedRect.size,
                           rotatedRect.angle);
  }

  cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
  drawContours(mask, std::vector<std::vector<cv::Point>>{biggestContour}, -1,
               cv::Scalar(255), -1);

  std::vector<cv::Point> ptsEdges;
  findNonZero(mask, ptsEdges);

  if (ptsEdges.size() >= 5) {
    const cv::RotatedRect rotatedRect = fitEllipse(ptsEdges);
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    log("Temps écoulé pour retrieveEllipse: " +
        std::to_string(elapsed.count()) + " secondes");
    return std::make_tuple(rotatedRect.center, rotatedRect.size,
                           rotatedRect.angle);
  }

  const auto end = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> elapsed = end - start;
  log("Temps écoulé pour retrieveEllipse: " + std::to_string(elapsed.count()) +
      " secondes");
  return emptyEllipse;
}
} // namespace subvision
