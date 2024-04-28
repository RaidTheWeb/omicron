#ifndef _ENGINE__ASSERTION_HPP
#define _ENGINE__ASSERTION_HPP

#include <stdio.h>

// XXX: TODO: Forcibly end a job on assertion failure.
// XXX: Consider safely ending a task whenever an assertion fails rather than crashing the entire engine.

#define ASSERT(cond, ...) ({ \
    if (!(cond)) { \
        printf("Assertion (" #cond ") failed in %s, function %s on line %d:\n\t", __FILE__, __FUNCTION__, __LINE__); \
        printf(__VA_ARGS__); \
        exit(1); \
    } \
})

#endif
