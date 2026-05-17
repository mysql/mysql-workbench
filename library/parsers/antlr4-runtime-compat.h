/*
 * Compatibility shims for ANTLR C++ generated files when the generator is
 * newer than the distro-provided runtime.
 */

#pragma once

#include <mutex>

#include <antlr4-runtime.h>

#if defined(ANTLRCPP_VERSION_MAJOR) && defined(ANTLRCPP_VERSION_MINOR)
#if ANTLRCPP_VERSION_MAJOR == 4 && ANTLRCPP_VERSION_MINOR < 13
namespace antlr4 {
namespace internal {
using OnceFlag = std::once_flag;
using std::call_once;
} // namespace internal
} // namespace antlr4
#endif
#endif
