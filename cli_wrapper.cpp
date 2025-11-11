#include "include/types.h"
#include "include/impact_detection.h"
#include "include/sheet_detection.h"
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
        static ImpactResults^ ProcessTargetImage(array<unsigned char>^ imageData, int width, int height) {
            // Convert managed array to native vector
            std::vector<unsigned char> nativeData(imageData->Length);
            Marshal::Copy((array<unsigned char>^)imageData, 0, IntPtr(nativeData.data()), imageData->Length);

            // Create OpenCV Mat from the data (RGBA format)
            cv::Mat mat(height, width, CV_8UC4, nativeData.data());
            cv::Mat bgrMat;
            cv::cvtColor(mat, bgrMat, cv::COLOR_RGBA2BGR);

            // Call native function
            subvision::ImpactResults nativeResults;
            bool success = subvision::retrieveImpacts(bgrMat, nativeResults);

            // Convert results to managed types
            ImpactResults^ managedResults = gcnew ImpactResults();

            if (success) {
                // Convert annotated image back to RGBA
                cv::Mat annotatedRGBA;
                cv::cvtColor(nativeResults.annotatedImage, annotatedRGBA, cv::COLOR_BGR2RGBA);

                // Copy image data to managed array
                int dataSize = annotatedRGBA.total() * annotatedRGBA.elemSize();
                managedResults->AnnotatedImageData = gcnew array<unsigned char>(dataSize);
                Marshal::Copy(IntPtr(annotatedRGBA.data), managedResults->AnnotatedImageData, 0, dataSize);
                managedResults->Width = annotatedRGBA.cols;
                managedResults->Height = annotatedRGBA.rows;
                managedResults->Channels = annotatedRGBA.channels();

                // Convert impacts
                for (const auto& impact : nativeResults.impacts) {
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
    };
}
