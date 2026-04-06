/**
 * @file cli_wrapper.cpp
 * @brief C++/CLI wrapper exposing the Subvision CV library to .NET
 * applications.
 *
 * Provides managed (.NET) classes that wrap the native C++ API, enabling
 * C# and other .NET languages to use Subvision CV for impact detection
 * and sheet coordinate extraction.
 *
 * ## Architecture
 *
 * The wrapper performs the following marshalling:
 * - **Input**: Managed `byte[]` arrays (RGBA pixel data) → native `cv::Mat`
 * (BGR)
 * - **Output**: Native `subvision::ImpactResults` → managed `ImpactResults^`
 * with copied image data and managed `Impact^` objects
 * - **Coordinates**: Managed `List<Point2f^>` ↔ native
 * `std::vector<cv::Point2f>`
 *
 * ## Memory Management
 *
 * - Input arrays are pinned (via `pin_ptr`) during processing to prevent GC
 * relocation
 * - Output image data is copied from native to managed heap via `Marshal::Copy`
 * - Native cv::Mat objects are automatically freed when they go out of scope
 *
 * ## .NET Usage Example
 *
 * ```csharp
 * using SubvisionNET;
 *
 * // Load an image as RGBA byte array
 * byte[] imageData = LoadImageAsRGBA("target_sheet.jpg", out int width, out int
 * height);
 *
 * // Detect impacts
 * var results = SubvisionCore.ProcessTargetImage(imageData, width, height,
 * null); foreach (var impact in results.Impacts)
 * {
 *     Console.WriteLine($"Score: {impact.Score}, Distance:
 * {impact.Distance}mm");
 * }
 *
 * // Get sheet coordinates
 * var coords = SubvisionCore.GetSheetCoordinates(imageData, width, height);
 * foreach (var pt in coords)
 * {
 *     Console.WriteLine($"Corner: ({pt.X}, {pt.Y})");
 * }
 * ```
 */

#include "include/impact_detection.h"
#include "include/logging.h"
#include "include/sheet_detection.h"
#include "include/types.h"
#include <msclr/marshal_cppstd.h>


using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

/// <summary>
/// Root namespace for the Subvision .NET wrapper library.
/// Contains managed classes that expose the native C++ computer vision
/// API for underwater target shooting sheet analysis.
/// </summary>
namespace SubvisionNET {

/// <summary>
/// Represents a single detected impact (shot) on a target.
/// </summary>
/// <remarks>
/// Maps to the native <c>subvision::Impact</c> struct.
/// Each impact contains scoring information computed from its
/// position relative to the target center.
/// </remarks>
public
ref class Impact {
public:
  /// <summary>Distance from the target center in millimeters.</summary>
  property int Distance;

  /// <summary>Computed score on the 0–570 federation scale.</summary>
  property int Score;

  /// <summary>Target zone identifier (0–4, or -1 for undefined).</summary>
  property int Zone;

  /// <summary>Angular position in degrees relative to target center.</summary>
  property float Angle;

  /// <summary>Number of impacts at this location (typically 1).</summary>
  property int Count;

  /// <summary>
  /// Constructs an Impact with all scoring properties.
  /// </summary>
  /// <param name="distance">Distance from center in millimeters.</param>
  /// <param name="score">Computed score value.</param>
  /// <param name="zone">Target zone identifier.</param>
  /// <param name="angle">Angular position in degrees.</param>
  /// <param name="count">Number of impacts.</param>
  Impact(int distance, int score, int zone, float angle, int count) {
    Distance = distance;
    Score = score;
    Zone = zone;
    Angle = angle;
    Count = count;
  }
};

/// <summary>
/// Represents a 2D point with floating-point coordinates.
/// </summary>
/// <remarks>
/// Maps to the native <c>cv::Point2f</c>. Used for sheet corner
/// coordinates returned as normalised percentages in [0, 1] range.
/// </remarks>
public
ref class Point2f {
public:
  /// <summary>X coordinate (normalised to [0, 1] for percentage
  /// coordinates).</summary>
  property float X;

  /// <summary>Y coordinate (normalised to [0, 1] for percentage
  /// coordinates).</summary>
  property float Y;

  /// <summary>
  /// Constructs a Point2f with the given coordinates.
  /// </summary>
  /// <param name="x">X coordinate.</param>
  /// <param name="y">Y coordinate.</param>
  Point2f(float x, float y) {
    X = x;
    Y = y;
  }
};

/// <summary>
/// Contains the results of impact detection processing.
/// </summary>
/// <remarks>
/// Maps to the native <c>subvision::ImpactResults</c>.
/// The annotated image data is returned as an RGBA byte array
/// that can be directly used to create a bitmap in .NET.
/// Memory for the image data is managed by the .NET garbage collector.
/// </remarks>
public
ref class ImpactResults {
public:
  /// <summary>RGBA pixel data of the annotated image.</summary>
  /// <remarks>
  /// Array length = Width × Height × Channels.
  /// The image has targets and impacts drawn on it.
  /// </remarks>
  property array<unsigned char> ^ AnnotatedImageData;

  /// <summary>Width of the annotated image in pixels.</summary>
  property int Width;

  /// <summary>Height of the annotated image in pixels.</summary>
  property int Height;

  /// <summary>Number of colour channels (always 4 for RGBA).</summary>
  property int Channels;

  /// <summary>List of detected impacts with their scores.</summary>
  property List<Impact ^> ^ Impacts;

  /// <summary>
  /// Constructs an empty ImpactResults with an initialised Impacts list.
  /// </summary>
  ImpactResults() { Impacts = gcnew List<Impact ^>(); }
};

/// <summary>
/// Main wrapper class exposing the Subvision CV native API to .NET.
/// </summary>
/// <remarks>
/// <para>
/// All methods are static and thread-safe for independent calls.
/// Image data must be provided as RGBA byte arrays.
/// </para>
/// <para>
/// <b>Native interop:</b> Input arrays are pinned during processing.
/// Output data is fully copied to managed memory — no native pointers
/// are retained after the call returns.
/// </para>
/// <para>
/// <b>Mapping to C++ API:</b>
/// <list type="bullet">
/// <item><c>ProcessTargetImage</c> → <c>subvision::retrieveImpacts()</c></item>
/// <item><c>GetSheetCoordinates</c> →
/// <c>subvision::getSheetCoordinates()</c></item>
/// <item><c>SetLoggingEnabled</c> →
/// <c>subvision::setLoggingEnabled()</c></item>
/// </list>
/// </para>
/// </remarks>
/// <example>
/// <code>
/// var results = SubvisionCore.ProcessTargetImage(imageData, width, height,
/// null); foreach (var impact in results.Impacts)
///     Console.WriteLine($"Score={impact.Score}");
/// </code>
/// </example>
public
ref class SubvisionCore {
public:
  /// <summary>
  /// Process a target image to detect and score all impacts.
  /// </summary>
  /// <remarks>
  /// <para>
  /// This is the main entry point for impact detection from .NET.
  /// The method performs the full Subvision pipeline: sheet detection,
  /// perspective correction, target localisation, impact detection,
  /// and scoring.
  /// </para>
  /// <para>
  /// <b>Colour conversion:</b> The input RGBA data is converted to BGR
  /// internally (OpenCV convention). Supports 1, 3, or 4 channel inputs.
  /// </para>
  /// <para>
  /// <b>Memory:</b> The input array is pinned (not copied) during processing.
  /// The output annotated image is fully copied to managed memory.
  /// </para>
  /// </remarks>
  /// <param name="imageData">RGBA pixel data as a byte array.</param>
  /// <param name="width">Image width in pixels.</param>
  /// <param name="height">Image height in pixels.</param>
  /// <param name="coordinates">Optional pre-computed sheet corner coordinates.
  /// Pass null for automatic sheet detection.</param>
  /// <returns>ImpactResults with annotated image and impact list,
  /// or null if input is invalid.</returns>
  static ImpactResults ^
      ProcessTargetImage(array<unsigned char> ^ imageData, int width,
                         int height, List<Point2f ^> ^ coordinates) {
        if (imageData == nullptr || width <= 0 || height <= 0)
          return nullptr;

        int length = imageData->Length;
        if (length == 0)
          return nullptr;

        // Calcul du pas (bytes par ligne) et du nombre de canaux
        int step = length / height;
        if (step <= 0)
          return nullptr;
        int channels = step / width;
        if (channels <= 0)
          return nullptr;

        int type;
        if (channels == 1)
          type = CV_8UC1;
        else if (channels == 3)
          type = CV_8UC3;
        else if (channels == 4)
          type = CV_8UC4;
        else
          return nullptr; // format non supporté

        // log type
        subvision::log("Image type detected: " + std::to_string(type) +
                       " with " + std::to_string(channels) +
                       " channels and step " + std::to_string(step));

        // Pinner le tableau managé et construire une cv::Mat qui utilise ces
        // données (avec step correct)
        pin_ptr<unsigned char> pinned = &imageData[0];
        unsigned char *dataPtr = pinned;
        cv::Mat mat(height, width, type, dataPtr, step);

        // Convertir en BGR attendu par le pipeline natif
        cv::Mat bgrMat;
        if (channels == 4) {
          cv::cvtColor(mat, bgrMat,
                       cv::COLOR_RGBA2BGR); // ajuster si vos données sont BGRA
        } else if (channels == 3) {
          cv::cvtColor(
              mat, bgrMat,
              cv::COLOR_RGB2BGR); // ajuster si vos données sont déjà BGR
        } else                    // 1 canal
        {
          cv::cvtColor(mat, bgrMat, cv::COLOR_GRAY2BGR);
        }

        // Convertir coordonnées managées -> natives
        std::vector<cv::Point2f> nativeCoords;
        if (coordinates != nullptr && coordinates->Count > 0) {
          nativeCoords.reserve(coordinates->Count);
          for each (Point2f ^ p in coordinates) {
            nativeCoords.emplace_back(p->X, p->Y);
          }
        }

        // Appel à la fonction native
        subvision::ImpactResults nativeResults;
        bool success =
            subvision::retrieveImpacts(bgrMat, nativeResults, nativeCoords);

        // Convertir résultats -> types managés
        ImpactResults ^ managedResults = gcnew ImpactResults();
        if (success) {
          cv::Mat annotatedRGBA;
          cv::cvtColor(nativeResults.annotatedImage, annotatedRGBA,
                       cv::COLOR_BGR2RGBA);

          int dataSize = static_cast<int>(annotatedRGBA.total() *
                                          annotatedRGBA.elemSize());
          managedResults->AnnotatedImageData =
              gcnew array<unsigned char>(dataSize);
          Marshal::Copy(IntPtr(annotatedRGBA.data),
                        managedResults->AnnotatedImageData, 0, dataSize);
          managedResults->Width = annotatedRGBA.cols;
          managedResults->Height = annotatedRGBA.rows;
          managedResults->Channels = annotatedRGBA.channels();

          for (const auto &impact : nativeResults.impacts) {
            Impact ^ managedImpact =
                gcnew Impact(impact.distance, impact.score, impact.zone,
                             impact.angle, impact.count);
            managedResults->Impacts->Add(managedImpact);
          }
        }

        return managedResults;
      }

      /// <summary>
      /// Detect the four corner coordinates of the shooting sheet.
      /// </summary>
      /// <remarks>
      /// <para>
      /// Detects the white shooting sheet in the image and returns its
      /// four corners as normalised percentage coordinates in [0, 1] range.
      /// These coordinates can be stored and reused with
      /// <see cref="ProcessTargetImage"/> to skip automatic detection.
      /// </para>
      /// <para>
      /// <b>Native mapping:</b> Calls <c>subvision::getSheetCoordinates()</c>.
      /// </para>
      /// </remarks>
      /// <param name="imageData">RGBA pixel data as a byte array.</param>
      /// <param name="width">Image width in pixels.</param>
      /// <param name="height">Image height in pixels.</param>
      /// <returns>List of 4 Point2f objects representing sheet corners
      /// in normalised coordinates.</returns>
      static List<Point2f ^> ^
      GetSheetCoordinates(array<unsigned char> ^ imageData, int width,
                          int height) {
        // Convert managed array to native vector
        std::vector<unsigned char> nativeData(imageData->Length);
        Marshal::Copy((array<unsigned char> ^) imageData, 0,
                      IntPtr(nativeData.data()), imageData->Length);

        // Create OpenCV Mat from the data (RGBA format)
        cv::Mat mat(height, width, CV_8UC4, nativeData.data());
        cv::Mat bgrMat;
        cv::cvtColor(mat, bgrMat, cv::COLOR_RGBA2BGR);

        // Call native function
        std::vector<cv::Point2f> nativePoints =
            subvision::getSheetCoordinates(bgrMat);

        // Convert to managed list
        List<Point2f ^> ^ managedPoints = gcnew List<Point2f ^>();
        for (const auto &pt : nativePoints) {
          managedPoints->Add(gcnew Point2f(pt.x, pt.y));
        }

        return managedPoints;
      }

      /// <summary>
      /// Enable or disable runtime logging.
      /// </summary>
      /// <remarks>
      /// When enabled, log messages from the native C++ pipeline are written
      /// to <c>System.Console</c>. Logging is disabled by default.
      /// <para>
      /// <b>Native mapping:</b> Calls <c>subvision::setLoggingEnabled()</c>.
      /// </para>
      /// </remarks>
      /// <param name="enabled">True to enable logging, false to
      /// disable.</param>
      static void SetLoggingEnabled(bool enabled) {
    subvision::setLoggingEnabled(enabled);
  }
};
} // namespace SubvisionNET