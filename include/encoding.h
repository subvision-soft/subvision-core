#ifndef SUBVISION_CORE_ENCODING_H
#define SUBVISION_CORE_ENCODING_H

#include <string>
#include <vector>

namespace subvision {
    // Caractères de base64
    extern const std::string base64_chars;

    // Vérifier si un caractère est en base64
    bool is_base64(unsigned char c);

    // Décoder une chaîne base64 en bytes
    std::vector<unsigned char> base64_decode(const std::string &encoded_string);

    // Encoder des bytes en chaîne base64
    std::string base64_encode(const unsigned char *bytes_to_encode, unsigned int in_len);
}

#endif //SUBVISION_CORE_ENCODING_H
