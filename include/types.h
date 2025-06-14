#ifndef SUBVISION_CV_TYPES_H
#define SUBVISION_CV_TYPES_H

#include <opencv2/opencv.hpp>
#include <tuple>

namespace subvision {
    using Ellipse = std::tuple<cv::Point2f, cv::Size2f, float>;

    struct Impact {
        int distance;
        int score;
        int zone;
        float angle;
        int count;

        Impact(int distance, int score, int zone, float angle, int count)
            : distance(distance), score(score), zone(zone), angle(angle), count(count) {}
    };

    struct ImpactResults {
        cv::Mat annotatedImage;
        std::vector<Impact> impacts;
    };
}

#endif //SUBVISION_CV_TYPES_H
