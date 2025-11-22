#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>
#include "include/types.h"
#include "include/impact_detection.h"
#include "include/sheet_detection.h"
#include "include/logging.h"

using namespace emscripten;

// Structure pour représenter un impact en JavaScript
struct JSImpact {
    int distance;
    int score;
    int zone;
    float angle;
    int count;

    // Conversion d'un Impact C++ vers JSImpact
    static JSImpact fromImpact(const subvision::Impact &impact) {
        JSImpact jsImpact;
        jsImpact.distance = impact.distance;
        jsImpact.score = impact.score;
        jsImpact.zone = impact.zone;
        jsImpact.angle = impact.angle;
        jsImpact.count = impact.count;
        return jsImpact;
    }
};

// Structure pour les résultats retournés à JavaScript
struct JSImpactResults {
    cv::Mat annotatedImage;
    val impacts = val::array();
};

template<typename T>
val getSheetCoordinates(int width, int height, const val &typedArray) {
    subvision::log("Start processing getSheetCoordinates with width: " + std::to_string(width) + ", height: " + std::to_string(height));
    std::vector<T> vec = convertJSArrayToNumberVector<T>(typedArray);
    subvision::log("Vector size: " + std::to_string(vec.size()));
    cv::Mat mat(height, width, CV_8UC4, vec.data());
    cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);

    subvision::log("Processing getSheetCoordinates with width: " + std::to_string(width) + ", height: " + std::to_string(height));
    auto points = subvision::getSheetCoordinates(mat);

    val jsArray = val::array();
    for (const auto &pt: points) {
        val jsPoint = val::object();
        jsPoint.set("x", pt.x);
        jsPoint.set("y", pt.y);
        jsArray.call<void>("push", jsPoint);
    }

    return jsArray;
}


// Fonction wrapper pour retrieveImpacts
template<typename T>
JSImpactResults processTargetImage(int width, int height, const val &typedArray) {
    subvision::ImpactResults results;
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<T> vec = convertJSArrayToNumberVector<T>(typedArray);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    subvision::log("Temps écoulé pour vecFromJSArray: " + std::to_string(elapsed.count()) + " secondes");
    cv::Mat mat(height, width, CV_8UC4, vec.data());
    cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);

    bool success = subvision::retrieveImpacts(mat, results);

    JSImpactResults jsResults;
    if (success) {
        auto annotated_image = results.annotatedImage;
        cv::cvtColor(annotated_image, annotated_image, cv::COLOR_BGR2RGBA);
        jsResults.annotatedImage = annotated_image;

        val impactArray = val::array();
        for (const auto& impact : results.impacts) {
            impactArray.call<void>("push", JSImpact::fromImpact(impact));
        }
        jsResults.impacts = impactArray;
    }


    return jsResults;
}

template<typename T>
val matData(const cv::Mat &mat) {
    return val(memory_view<T>((mat.total() * mat.elemSize()) / sizeof(T),
                              (T *) mat.data));
}

// Définition des liaisons Emscripten
EMSCRIPTEN_BINDINGS (subvision_module) {
    register_vector<uchar>("vector_uchar");
    register_vector<cv::Point2f>("vector_point2f");
    class_<cv::Point2f>("Point2f")
            .constructor<float, float>()
            .property("x", &cv::Point2f::x)
            .property("y", &cv::Point2f::y);

    class_<cv::Mat>("Mat")
            .property("rows", &cv::Mat::rows)
            .property("columns", &cv::Mat::cols)
            .property("data", &matData<unsigned char>);

    value_object<JSImpact>("Impact")
            .field("distance", &JSImpact::distance)
            .field("score", &JSImpact::score)
            .field("zone", &JSImpact::zone)
            .field("angle", &JSImpact::angle)
            .field("count", &JSImpact::count);

    register_vector<JSImpact>("ImpactVector");

    value_object<JSImpactResults>("ImpactResults")
            .field("annotatedImage", &JSImpactResults::annotatedImage)
            .field("impacts", &JSImpactResults::impacts);

    function("processTargetImage", &processTargetImage<unsigned char>);
    function("getSheetCoordinates", &getSheetCoordinates<unsigned char>);
    function("setLoggingEnabled", &subvision::setLoggingEnabled);
}
