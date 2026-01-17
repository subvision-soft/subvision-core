#include "include/types.h"
#include "include/impact_detection.h"
#include "include/sheet_detection.h"
#include "include/logging.h"
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

namespace SubvisionNET {
    // .NET representation of Impact
    public ref class Impact {
    public:
        property int Distance;
        property int Score;
        property int Zone;
        property float Angle;
        property int Count;

        Impact(int distance, int score, int zone, float angle, int count) {
            Distance = distance;
            Score = score;
            Zone = zone;
            Angle = angle;
            Count = count;
        }
    };

    // .NET representation of Point2f
    public ref class Point2f {
    public:
        property float X;
        property float Y;

        Point2f(float x, float y) {
            X = x;
            Y = y;
        }
    };

    // .NET representation of Impact Results
    public ref class ImpactResults {
    public:
        property array<unsigned char>^ AnnotatedImageData;
        property int Width;
        property int Height;
        property int Channels;
        property List<Impact^>^ Impacts;

        ImpactResults() {
            Impacts = gcnew List<Impact^>();
        }
    };

    // Main wrapper class
    public ref class SubvisionCore {
    public:
        // Process target image and detect impacts
        // imageData: RGBA image data as byte array
        // width: image width
        // height: image height
        // C++
        static ImpactResults^ ProcessTargetImage (array<unsigned char>^ imageData, int width, int height, List<Point2f^>^ coordinates)
        {
            if (imageData == nullptr || width <= 0 || height <= 0) return nullptr;

            int length = imageData->Length;
            if (length == 0) return nullptr;

            // Calcul du pas (bytes par ligne) et du nombre de canaux
            int step = length / height;
            if (step <= 0) return nullptr;
            int channels = step / width;
            if (channels <= 0) return nullptr;

            int type;
            if (channels == 1) type = CV_8UC1;
            else if (channels == 3) type = CV_8UC3;
            else if (channels == 4) type = CV_8UC4;
            else return nullptr; // format non supporté

            // Pinner le tableau managé et construire une cv::Mat qui utilise ces données (avec step correct)
            pin_ptr<unsigned char> pinned = &imageData[0];
            unsigned char* dataPtr = pinned;
            cv::Mat mat(height, width, type, dataPtr, step);

            // Convertir en BGR attendu par le pipeline natif
            cv::Mat bgrMat;
            if (channels == 4)
            {
                cv::cvtColor(mat, bgrMat, cv::COLOR_RGBA2BGR); // ajuster si vos données sont BGRA
            }
            else if (channels == 3)
            {
                cv::cvtColor(mat, bgrMat, cv::COLOR_RGB2BGR); // ajuster si vos données sont déjà BGR
            }
            else // 1 canal
            {
                cv::cvtColor(mat, bgrMat, cv::COLOR_GRAY2BGR);
            }

            // Convertir coordonnées managées -> natives
            std::vector<cv::Point2f> nativeCoords;
            if (coordinates != nullptr && coordinates->Count > 0)
            {
                nativeCoords.reserve(coordinates->Count);
                for each (Point2f^ p in coordinates)
                {
                    nativeCoords.emplace_back(p->X, p->Y);
                }
            }

            // Appel à la fonction native
            subvision::ImpactResults nativeResults;
            bool success = subvision::retrieveImpacts(bgrMat, nativeResults, nativeCoords);

            // Convertir résultats -> types managés
            ImpactResults^ managedResults = gcnew ImpactResults();
            if (success)
            {
                cv::Mat annotatedRGBA;
                cv::cvtColor(nativeResults.annotatedImage, annotatedRGBA, cv::COLOR_BGR2RGBA);

                int dataSize = static_cast<int>(annotatedRGBA.total() * annotatedRGBA.elemSize());
                managedResults->AnnotatedImageData = gcnew array<unsigned char>(dataSize);
                Marshal::Copy(IntPtr(annotatedRGBA.data), managedResults->AnnotatedImageData, 0, dataSize);
                managedResults->Width = annotatedRGBA.cols;
                managedResults->Height = annotatedRGBA.rows;
                managedResults->Channels = annotatedRGBA.channels();

                for (const auto& impact : nativeResults.impacts)
                {
                    Impact^ managedImpact = gcnew Impact(
                        impact.distance,
                        impact.score,
                        impact.zone,
                        impact.angle,
                        impact.count
                    );
                    managedResults->Impacts->Add(managedImpact);
                }
            }

            return managedResults;
        }


        // Get sheet coordinates from image
        // imageData: RGBA image data as byte array
        // width: image width
        // height: image height
        static List<Point2f^>^ GetSheetCoordinates(array<unsigned char>^ imageData, int width, int height) {
            // Convert managed array to native vector
            std::vector<unsigned char> nativeData(imageData->Length);
            Marshal::Copy((array<unsigned char>^)imageData, 0, IntPtr(nativeData.data()), imageData->Length);

            // Create OpenCV Mat from the data (RGBA format)
            cv::Mat mat(height, width, CV_8UC4, nativeData.data());
            cv::Mat bgrMat;
            cv::cvtColor(mat, bgrMat, cv::COLOR_RGBA2BGR);

            // Call native function
            std::vector<cv::Point2f> nativePoints = subvision::getSheetCoordinates(bgrMat);

            // Convert to managed list
            List<Point2f^>^ managedPoints = gcnew List<Point2f^>();
            for (const auto& pt : nativePoints) {
                managedPoints->Add(gcnew Point2f(pt.x, pt.y));
            }

            return managedPoints;
        }

        // Enable or disable logging
        // enabled: true to enable logging, false to disable
        static void SetLoggingEnabled(bool enabled) {
            subvision::setLoggingEnabled(enabled);
        }
    };
}