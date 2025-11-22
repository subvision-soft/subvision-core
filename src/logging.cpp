#include "../include/logging.h"
#include <iostream>

bool subvision::g_loggingEnabled = false;   // default: enabled

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

void subvision::setLoggingEnabled(bool enabled) {
    g_loggingEnabled = enabled;
}

void subvision::log(const std::string &msg) {
    if (!g_loggingEnabled) return;   // ZERO COST WHEN DISABLED

#if defined(__EMSCRIPTEN__)
    emscripten_log(EM_LOG_CONSOLE, "%s", msg.c_str());

#elif defined(_MANAGED) || defined(__CLR_VER)
    System::Console::WriteLine(gcnew System::String(msg.c_str()));

#else
    std::cout << msg << std::endl;
#endif
}
