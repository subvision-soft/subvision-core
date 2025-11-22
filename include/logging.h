#pragma once
#include <string>

namespace subvision {

    extern bool g_loggingEnabled;

    void setLoggingEnabled(bool enabled);
    void log(const std::string &msg);

}
