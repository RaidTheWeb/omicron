#ifndef _ENGINE__ASSERTION_HPP
#define _ENGINE__ASSERTION_HPP

#include <stdio.h>

#define ASSERT(cond, ...) ({ \
    if (!(cond)) { \
        printf("Assertion (" #cond ") failed in %s, function %s on line %d:\n\t", __FILE__, __FUNCTION__, __LINE__); \
        printf(__VA_ARGS__); \
        abort(); \
    } \
})

#endif
