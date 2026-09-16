/**
 * @file logging.h
 * @brief Cross-platform logging utilities for the Subvision CV library.
 *
 * Provides a simple logging mechanism that adapts to the build target:
 * - **Emscripten**: uses `emscripten_log()` to write to the browser console
 * - **C++/CLI (.NET)**: uses `System::Console::WriteLine()`
 * - **Native C++**: uses `std::cout`
 *
 * Logging is disabled by default and can be enabled at runtime
 * via setLoggingEnabled(). When disabled, log calls have zero overhead.
 */

#pragma once
#include <string>

namespace subvision {

/**
 * @brief Global flag controlling whether log messages are emitted.
 *
 * Defaults to `false`. Set via setLoggingEnabled().
 */
extern bool g_loggingEnabled;

/**
 * @brief Enable or disable runtime logging.
 *
 * @param enabled `true` to enable log output, `false` to suppress.
 *
 * @note In WebAssembly builds, this function is exported to JavaScript
 *       via `Module.setLoggingEnabled(true)`.
 *
 * @code
 * subvision::setLoggingEnabled(true);  // C++
 * Module.setLoggingEnabled(true);      // JavaScript
 * SubvisionCore.SetLoggingEnabled(true); // C#
 * @endcode
 */
void setLoggingEnabled(bool enabled);

/**
 * @brief Write a log message to the platform-appropriate output.
 *
 * Does nothing if logging is disabled (g_loggingEnabled == false).
 *
 * @param msg The message string to log.
 */
void log(const std::string &msg);

} // namespace subvision
