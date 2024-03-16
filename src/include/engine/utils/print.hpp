#ifndef _ENGINE__UTILS__PRINT_HPP
#define _ENGINE__UTILS__PRINT_HPP

#include <stdarg.h>
#include <stdio.h>

namespace OUtils {

static inline void print(const char *fmt, ...) {
#ifdef OMICRON_DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}

}

#endif
