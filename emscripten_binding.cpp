#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "include/types.h"
#include "include/impact_detection.h"

using namespace emscripten;

// Structure pour représenter un impact en JavaScript
struct JSImpact {
    int distance;
    int score;
    int zone;
    float angle;
    int count;

    // Conversion d'un Impact C++ vers JSImpact
    static JSImpact fromImpact(const subvision::Impact& impact) {
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
    std::vector<JSImpact> impacts;
};

// Fonction wrapper pour retrieveImpacts
template<typename T>
JSImpactResults processTargetImage(int width, int height, const val& typedArray) {
    subvision::ImpactResults results;
    std::vector<T> vec = vecFromJSArray<T>(typedArray);
    cv::Mat mat(width,height, CV_8UC4, vec.data());
    bool success = subvision::retrieveImpacts(mat, results);

    JSImpactResults jsResults;
    if (success) {
        jsResults.annotatedImage = results.annotatedImage;

        // Convertir les impacts en format JS
        for (const auto& impact : results.impacts) {
            jsResults.impacts.push_back(JSImpact::fromImpact(impact));
        }
    }

    return jsResults;
}
template<typename T>
val matData(const cv::Mat& mat)
{
    return val(memory_view<T>((mat.total()*mat.elemSize())/sizeof(T),
                           (T*)mat.data));
}

// Définition des liaisons Emscripten
EMSCRIPTEN_BINDINGS(subvision_module) {
    register_vector<uchar>("vector_uchar");
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
}
