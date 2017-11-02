#pragma once

#include "export.hpp"

#if defined(__GNUC__)
extern "C" OTR_COMPAT_EXPORT bool __attribute__ ((noinline)) nanp(double value);
#elif defined(_WIN32)
extern "C" OTR_COMPAT_EXPORT __declspec(noinline) bool nanp(double value);
#else
extern "C" OTR_COMPAT_EXPORT bool nanp(double value);
#endif
