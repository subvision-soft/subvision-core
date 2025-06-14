#include <iostream>
#include <fstream>
#include <vector>

#include "include/encoding.h"
#include "include/impact_detection.h"
#include "include/types.h"
#include <opencv2/opencv.hpp>

int main() {
    // Charger l'image depuis le fichier
    std::ifstream file("C:\\Users\\Paul\\CLionProjects\\subvision-cv\\cropped_sheet.jpg", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Erreur: Impossible d'ouvrir cropped_sheet.jpg" << std::endl;
        return 1;
    }

    // Obtenir la taille du fichier
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Lire les données de l'image
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Erreur: Impossible de lire cropped_sheet.jpg" << std::endl;
        return 1;
    }

    // Encoder l'image en base64

    // Appeler retrieveImpacts
    subvision::ImpactResults results;
// start recording time for performance measurement

    auto start = std::chrono::high_resolution_clock::now();

    bool success = subvision::retrieveImpacts(cv::imread("C:\\Users\\Paul\\CLionProjects\\subvision-cv\\cropped_sheet.jpg"), results);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Temps écoulé pour retrieveImpacts: " << elapsed.count() << " secondes" << std::endl;

    if (success) {
        std::cout << "Traitement réussi!" << std::endl;
        std::cout << "Nombre d'impacts détectés: " << results.impacts.size() << std::endl;
        // Afficher les informations sur chaque impact
        for (size_t i = 0; i < results.impacts.size(); ++i) {
            const auto& impact = results.impacts[i];
            std::cout << "Impact " << (i+1) << ":" << std::endl;
            std::cout << "  Zone: " << impact.zone << std::endl;
            std::cout << "  Score: " << impact.score << std::endl;
            std::cout << "  Distance: " << impact.distance << std::endl;
            std::cout << "  Angle: " << impact.angle << " degrés" << std::endl;
        }
    } else {
        std::cerr << "Échec du traitement de l'image" << std::endl;
        return 1;
    }

    return 0;
}
