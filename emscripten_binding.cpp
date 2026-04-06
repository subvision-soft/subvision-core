/**
 * @file emscripten_binding.cpp
 * @brief WebAssembly bindings for the Subvision CV library via Emscripten.
 *
 * Exposes the core Subvision functionality to JavaScript through Emscripten's
 * embind system. This file handles:
 * - RGBA → BGR colour space conversion (browser images are RGBA)
 * - Typed array marshalling between JavaScript and C++
 * - Result packaging as JavaScript-friendly objects
 *
 * ## Exported JavaScript API
 *
 * | C++ Function            | JavaScript Function                    |
 * Description                           |
 * |-------------------------|----------------------------------------|---------------------------------------|
 * | `processTargetImage<T>` | `Module.processTargetImage(w, h, arr)` | Detect
 * and score all impacts          | | `getSheetCoordinates<T>`|
 * `Module.getSheetCoordinates(w, h, arr)`| Detect sheet corner coordinates | |
 * `setLoggingEnabled`     | `Module.setLoggingEnabled(bool)`       |
 * Enable/disable console logging        |
 *
 * ## JavaScript Usage Example
 *
 * @code{.js}
 * import SubvisionCV from './subvision_core_es6.js';
 *
 * const module = await SubvisionCV();
 *
 * // Get image data from a canvas
 * const canvas = document.getElementById('myCanvas');
 * const ctx = canvas.getContext('2d');
 * const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
 *
 * // Process the target image
 * const results = module.processTargetImage(canvas.width, canvas.height,
 * imageData.data); console.log('Number of impacts:', results.impacts.size());
 *
 * for (let i = 0; i < results.impacts.size(); i++) {
 *     const impact = results.impacts.get(i);
 *     console.log(`Impact: score=${impact.score},
 * distance=${impact.distance}`);
 * }
 *
 * // Get sheet coordinates
 * const coords = module.getSheetCoordinates(canvas.width, canvas.height,
 * imageData.data); for (let i = 0; i < coords.size(); i++) { const pt =
 * coords.get(i); console.log(`Corner: (${pt.x}, ${pt.y})`);
 * }
 * @endcode
 */

#include "include/impact_detection.h"
#include "include/logging.h"
#include "include/sheet_detection.h"
#include "include/types.h"
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>


using namespace emscripten;

/**
 * @brief JavaScript-friendly representation of an impact result.
 *
 * Mirrors the native subvision::Impact struct but is designed for
 * use with Emscripten's value_object binding. Exported to JavaScript
 * as the `Impact` type.
 *
 * @note In JavaScript, access fields directly: `impact.distance`,
 * `impact.score`, etc.
 */
struct JSImpact {
  int distance; ///< Distance from center in millimeters.
  int score;    ///< Computed score (0–570).
  int zone;     ///< Target zone identifier.
  float angle;  ///< Angular position in degrees.
  int count;    ///< Number of impacts at this location.

  /**
   * @brief Convert a native C++ Impact to a JSImpact.
   *
   * @param impact The native Impact to convert.
   * @return A JSImpact with identical field values.
   */
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

/**
 * @brief Container for impact processing results returned to JavaScript.
 *
 * Exported as the `ImpactResults` type in JavaScript. Contains the
 * annotated image as a cv::Mat and an array of Impact objects.
 *
 * JavaScript usage:
 * @code{.js}
 * const results = module.processTargetImage(width, height, data);
 * const mat = results.annotatedImage;
 * const imageBytes = mat.data;  // Uint8Array of RGBA pixels
 * const impacts = results.impacts;  // ImpactVector
 * @endcode
 */
struct JSImpactResults {
  cv::Mat annotatedImage; ///< Annotated image with targets and impacts drawn
                          ///< (RGBA format).
  val impacts = val::array(); ///< JavaScript array of JSImpact objects.
};

/**
 * @brief Detect sheet corner coordinates from a JavaScript image buffer.
 *
 * Converts the incoming RGBA typed array to a BGR cv::Mat, calls the
 * native getSheetCoordinates(), and returns the result as a JavaScript
 * array of {x, y} point objects.
 *
 * @tparam T Pixel data type (typically `unsigned char`).
 * @param width      Image width in pixels.
 * @param height     Image height in pixels.
 * @param typedArray JavaScript Uint8Array containing RGBA pixel data.
 * @return JavaScript array of point objects with `x` and `y` properties
 *         (normalised percentage coordinates in [0, 1]).
 *
 * JavaScript usage:
 * @code{.js}
 * const coords = Module.getSheetCoordinates(width, height, imageData.data);
 * @endcode
 */
template <typename T>
val getSheetCoordinates(int width, int height, const val &typedArray) {
  subvision::log("Start processing getSheetCoordinates with width: " +
                 std::to_string(width) + ", height: " + std::to_string(height));
  std::vector<T> vec = convertJSArrayToNumberVector<T>(typedArray);
  subvision::log("Vector size: " + std::to_string(vec.size()));
  cv::Mat mat(height, width, CV_8UC4, vec.data());
  cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);

  subvision::log("Processing getSheetCoordinates with width: " +
                 std::to_string(width) + ", height: " + std::to_string(height));
  auto points = subvision::getSheetCoordinates(mat);

  val jsArray = val::array();
  for (const auto &pt : points) {
    val jsPoint = val::object();
    jsPoint.set("x", pt.x);
    jsPoint.set("y", pt.y);
    jsArray.call<void>("push", jsPoint);
  }

  return jsArray;
}

/**
 * @brief Process a target image from JavaScript and return impact results.
 *
 * Main entry point for WebAssembly impact detection. Converts the incoming
 * RGBA typed array to BGR, runs the full detection pipeline, and packages
 * results as JavaScript-friendly objects.
 *
 * @tparam T Pixel data type (typically `unsigned char`).
 * @param width      Image width in pixels.
 * @param height     Image height in pixels.
 * @param typedArray JavaScript Uint8Array containing RGBA pixel data.
 * @return JSImpactResults containing the annotated image (RGBA) and impact
 * array.
 *
 * @note The annotated image is converted back to RGBA before returning
 *       so it can be directly drawn to a canvas.
 *
 * JavaScript usage:
 * @code{.js}
 * const results = Module.processTargetImage(canvas.width, canvas.height,
 * imageData.data);
 * @endcode
 */
template <typename T>
JSImpactResults processTargetImage(int width, int height,
                                   const val &typedArray) {
  subvision::ImpactResults results;
  auto start = std::chrono::high_resolution_clock::now();

  std::vector<T> vec = convertJSArrayToNumberVector<T>(typedArray);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  subvision::log("Temps écoulé pour vecFromJSArray: " +
                 std::to_string(elapsed.count()) + " secondes");
  cv::Mat mat(height, width, CV_8UC4, vec.data());
  cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);

  bool success = subvision::retrieveImpacts(mat, results);

  JSImpactResults jsResults;
  if (success) {
    auto annotated_image = results.annotatedImage;
    cv::cvtColor(annotated_image, annotated_image, cv::COLOR_BGR2RGBA);
    jsResults.annotatedImage = annotated_image;

    val impactArray = val::array();
    for (const auto &impact : results.impacts) {
      impactArray.call<void>("push", JSImpact::fromImpact(impact));
    }
    jsResults.impacts = impactArray;
  }

  return jsResults;
}

/**
 * @brief Extract raw pixel data from a cv::Mat as a JavaScript typed array
 * view.
 *
 * Creates a memory_view over the Mat's data buffer, allowing JavaScript
 * to read the pixel data directly without copying.
 *
 * @tparam T Element type for the memory view (typically `unsigned char`).
 * @param mat The cv::Mat to expose.
 * @return An Emscripten val wrapping a typed array view of the Mat's data.
 *
 * @warning The returned view becomes invalid if the Mat is deallocated or
 * reallocated.
 */
template <typename T> val matData(const cv::Mat &mat) {
  return val(memory_view<T>((mat.total() * mat.elemSize()) / sizeof(T),
                            (T *)mat.data));
}

/**
 * @defgroup emscripten_bindings Emscripten WASM Bindings
 * @brief JavaScript API bindings for the Subvision WebAssembly module.
 *
 * These bindings export the following types and functions to JavaScript:
 *
 * **Types:**
 * - `Point2f` — 2D point with `x`, `y` float properties
 * - `Mat` — OpenCV matrix with `rows`, `columns`, `data` properties
 * - `Impact` — Impact result with `distance`, `score`, `zone`, `angle`, `count`
 * - `ImpactResults` — Contains `annotatedImage` (Mat) and `impacts` (array)
 *
 * **Functions:**
 * - `processTargetImage(width, height, typedArray)` — Full impact detection
 * - `getSheetCoordinates(width, height, typedArray)` — Sheet corner detection
 * - `setLoggingEnabled(bool)` — Toggle console logging
 *
 * **Vectors:**
 * - `vector_uchar` — std::vector<unsigned char>
 * - `vector_point2f` — std::vector<cv::Point2f>
 * - `ImpactVector` — std::vector<JSImpact>
 * @{
 */

// Définition des liaisons Emscripten
EMSCRIPTEN_BINDINGS(subvision_module) {
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

/** @} */ // end of emscripten_bindings group
